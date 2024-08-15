#include "Renderer.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <memory>
#include <vk_mem_alloc.h>

#include "DescriptorManager.h"
#include "MaterialManager.h"
#include "ModelManager.h"
#include "RenderPasses/GridRenderPass.h"
#include "RenderPasses/ImGuiRenderPass.h"
#include "RenderPasses/MainRenderPass.h"
#include "RenderPasses/ShadowRenderPass.h"
#include "Scene.h"
#include "VulkanDebugUtils.h"
#include "SlimeWindow.h"
#include "VulkanContext.h"
#include "VulkanUtil.h"

#define MAX_FRAMES_IN_FLIGHT 2

Renderer::Renderer(vkb::Device& device)
      : m_device(device)
{
}

Renderer::~Renderer()
{
	// Destroy synchronization objects
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_disp->destroySemaphore(m_availableSemaphores[i], nullptr);
		m_disp->destroySemaphore(m_finishedSemaphore[i], nullptr);
		m_disp->destroyFence(m_inFlightFences[i], nullptr);
	}

	for (auto& image_view: m_swapchainImageViews)
	{
		m_disp->destroyImageView(image_view, nullptr);
	}

	m_renderPassManager.Cleanup(*m_disp, m_allocator);
	CleanupDepthImage();

	vkb::destroy_swapchain(m_swapchain);
}

void Renderer::SetUp(vkb::DispatchTable* disp, VmaAllocator allocator, VkSurfaceKHR* surface, VulkanDebugUtils* debugUtils, SlimeWindow* window, ShaderManager* shaderManager, MaterialManager* materialManager, ModelManager* modelManager, DescriptorManager* descriptorManager, VkCommandPool commandPool)
{
	m_disp = disp;
	m_allocator = allocator;
	m_surface = surface;
	m_debugUtils = debugUtils;
	m_descriptorManager = descriptorManager;
	m_modelManager = modelManager;
	m_commandPool = commandPool;
	m_materialManager = materialManager;

	GetQueues();
	CreateRenderCommandBuffers();
	CreateSwapchain(window);
	InitSyncObjects();
	CreateDepthImage();
	SetupRenderPasses(shaderManager);
}

int Renderer::RenderFrame(ModelManager& modelManager, SlimeWindow* window, Scene* scene)
{
	// Wait for the frame to be finished
	VK_CHECK(m_disp->waitForFences(1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX));

	uint32_t imageIndex;
	VkResult result = m_disp->acquireNextImageKHR(m_swapchain, UINT64_MAX, m_availableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		if (CreateSwapchain(window) != 0)
			return -1;
		return 0;
	}
	else
	{
		VK_CHECK(result);
	}

	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (m_imageInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		m_disp->waitForFences(1, &m_imageInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	// Mark the image as now being in use by this frame
	m_imageInFlight[imageIndex] = m_inFlightFences[m_currentFrame];

	// Begin command buffer recording
	VkCommandBuffer cmd = m_renderCommandBuffers[imageIndex];

	if (Draw(cmd, m_commandPool, scene, imageIndex) != 0)
		return -1;

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore wait_semaphores[] = { m_availableSemaphores[m_currentFrame] };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd;

	VkSemaphore signal_semaphores[] = { m_finishedSemaphore[m_currentFrame] };
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;

	m_disp->resetFences(1, &m_inFlightFences[m_currentFrame]);

	m_debugUtils->BeginQueueDebugMarker(m_graphicsQueue, "FrameSubmission", debugUtil_FrameSubmission);
	VK_CHECK(m_disp->queueSubmit(m_graphicsQueue, 1, &submit_info, m_inFlightFences[m_currentFrame]));
	m_debugUtils->EndQueueDebugMarker(m_graphicsQueue);

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphores;

	VkSwapchainKHR swapchains[] = { m_swapchain };
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swapchains;
	present_info.pImageIndices = &imageIndex;

	result = m_disp->queuePresentKHR(m_presentQueue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		if (CreateSwapchain(window) != 0)
			return -1;
	}
	else
	{
		VK_CHECK(result);
	}

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return 0;
}

int Renderer::Draw(VkCommandBuffer& cmd, VkCommandPool commandPool, Scene* scene, uint32_t imageIndex)
{
	if (SlimeUtil::BeginCommandBuffer(*m_disp, cmd) != 0)
		return -1;

	Camera* camera = scene->m_entityManager.GetEntityByName("MainCamera")->GetComponentPtr<Camera>();

	// Transition images to appropriate layouts
	TransitionImages(m_graphicsQueue, commandPool, m_swapchainImages[imageIndex]);

	// Execute all render passes
	m_renderPassManager.ExecutePasses(*m_disp, cmd, *m_debugUtils, m_swapchain, m_swapchainImageViews[imageIndex], m_depthImageView, scene, camera);

	// Transition color image to present src layout
	SlimeUtil::TransitionImageLayout(*m_disp, m_graphicsQueue, commandPool, m_swapchainImages[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	if (SlimeUtil::EndCommandBuffer(*m_disp, cmd) != 0)
		return -1;

	// Handle multi-viewport rendering
	HandleMultiViewportRendering();

	return 0;
}

void Renderer::SetupRenderPasses(ShaderManager* shaderManager)
{
	auto shadowPass = std::make_shared<ShadowRenderPass>(*m_modelManager, m_allocator, m_commandPool, m_graphicsQueue);
	m_renderPassManager.AddPass(shadowPass);

	auto mainPass = std::make_shared<MainRenderPass>(shadowPass, m_materialManager, m_modelManager, m_allocator, m_commandPool, m_graphicsQueue, m_descriptorManager);
	m_renderPassManager.AddPass(mainPass);

	auto gridPass = std::make_shared<GridRenderPass>();
	m_renderPassManager.AddPass(gridPass);

	auto imguiPass = std::make_shared<ImGuiRenderPass>();
	m_renderPassManager.AddPass(imguiPass);

	m_renderPassManager.Setup(*m_disp, m_allocator, m_swapchain, shaderManager, *m_debugUtils);
}

void Renderer::TransitionImages(VkQueue graphicsQueue, VkCommandPool commandPool, VkImage swapchainImage)
{
	// Transition color image to color attachment optimal
	SlimeUtil::TransitionImageLayout(*m_disp, graphicsQueue, commandPool, swapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	// Transition depth image to depth attachment optimal
	SlimeUtil::TransitionImageLayout(*m_disp, graphicsQueue, commandPool, m_depthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
}

void Renderer::HandleMultiViewportRendering()
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

int Renderer::CreateRenderCommandBuffers()
{
	spdlog::debug("Recording command buffers...");
	m_renderCommandBuffers.resize(m_swapchain.image_count + 10);

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = m_commandPool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = static_cast<uint32_t>(m_renderCommandBuffers.size());

	VK_CHECK(m_disp->allocateCommandBuffers(&alloc_info, m_renderCommandBuffers.data()));

	int index = 0;
	for (auto& cmd: m_renderCommandBuffers)
	{
		m_debugUtils->SetObjectName(cmd, std::string("Render Command Buffer: " + std::to_string(index++)));
	}

	return 0;
}

int Renderer::InitSyncObjects()
{
	spdlog::debug("Initializing synchronization objects...");
	// Create synchronization objects
	m_availableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_finishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_imageInFlight.resize(m_swapchain.image_count, VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VK_CHECK(m_disp->createSemaphore(&semaphore_info, nullptr, &m_availableSemaphores[i]));
		VK_CHECK(m_disp->createSemaphore(&semaphore_info, nullptr, &m_finishedSemaphore[i]));
		VK_CHECK(m_disp->createFence(&fence_info, nullptr, &m_inFlightFences[i]));
	}

	return 0;
}

int Renderer::GetQueues()
{
	auto gq = m_device.get_queue(vkb::QueueType::graphics);
	if (!gq.has_value())
	{
		spdlog::error("failed to get graphics queue: {}", gq.error().message());
		return -1;
	}
	m_graphicsQueue = gq.value();

	auto pq = m_device.get_queue(vkb::QueueType::present);
	if (!pq.has_value())
	{
		spdlog::error("failed to get present queue: {}", pq.error().message());
		return -1;
	}
	m_presentQueue = pq.value();

	m_debugUtils->SetObjectName(m_graphicsQueue, "GraphicsQueue");
	m_debugUtils->SetObjectName(m_presentQueue, "PresentQueue");
	return 0;
}

int Renderer::CreateSwapchain(SlimeWindow* window)
{
	spdlog::debug("Creating swapchain...");
	m_disp->deviceWaitIdle();

	vkb::SwapchainBuilder swapchain_builder(m_device, *m_surface);

	auto swap_ret = swapchain_builder.use_default_format_selection()
	                        .set_desired_format(VkSurfaceFormatKHR{ .format = VK_FORMAT_B8G8R8A8_UNORM, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
	                        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // Use vsync present mode
	                        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	                        .set_old_swapchain(m_swapchain)
	                        .set_desired_extent(window->GetWidth(), window->GetHeight())
	                        .build();

	if (!swap_ret)
	{
		spdlog::error("Failed to create swapchain: {}", swap_ret.error().message());
		return -1;
	}

	vkb::destroy_swapchain(m_swapchain);
	m_swapchain = swap_ret.value();

	// Delete old image views
	for (auto& image_view: m_swapchainImageViews)
	{
		m_disp->destroyImageView(image_view, nullptr);
	}

	m_swapchainImages = m_swapchain.get_images().value();
	m_swapchainImageViews = m_swapchain.get_image_views().value();

	for (size_t i = 0; i < m_swapchainImages.size(); i++)
	{
		m_debugUtils->SetObjectName(m_swapchainImages[i], "SwapchainImage_" + std::to_string(i));
		m_debugUtils->SetObjectName(m_swapchainImageViews[i], "SwapchainImageView_" + std::to_string(i));
	}

	CreateDepthImage();

	return 0;
}

VkQueue Renderer::GetGraphicsQueue() const
{
	return m_graphicsQueue;
}

vkb::Swapchain& Renderer::GetSwapchain()
{
	return m_swapchain;
}

//
/// DEPTH TESTING ///////////////////////////////////
//
void Renderer::CreateDepthImage()
{
	spdlog::debug("Creating depth image");

	// Clean up old depth image and image view
	if (m_depthImage)
	{
		CleanupDepthImage();
	}

	// Create the depth image
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT; // Or VK_FORMAT_D32_SFLOAT_S8_UINT if you need stencil
	VkImageCreateInfo depthImageInfo = {};
	depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
	depthImageInfo.extent.width = m_swapchain.extent.width;
	depthImageInfo.extent.height = m_swapchain.extent.height;
	depthImageInfo.extent.depth = 1;
	depthImageInfo.mipLevels = 1;
	depthImageInfo.arrayLayers = 1;
	depthImageInfo.format = depthFormat;
	depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo depthAllocInfo = {};
	depthAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	SlimeUtil::CreateImage("DepthImage", m_allocator, &depthImageInfo, &depthAllocInfo, m_depthImage, m_depthImageAllocation);
	m_debugUtils->SetObjectName(m_depthImage, "DepthImage");

	// Create the depth image view
	VkImageViewCreateInfo depthImageViewInfo = {};
	depthImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthImageViewInfo.image = m_depthImage;
	depthImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthImageViewInfo.format = depthFormat;
	depthImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthImageViewInfo.subresourceRange.baseMipLevel = 0;
	depthImageViewInfo.subresourceRange.levelCount = 1;
	depthImageViewInfo.subresourceRange.baseArrayLayer = 0;
	depthImageViewInfo.subresourceRange.layerCount = 1;

	VK_CHECK(m_disp->createImageView(&depthImageViewInfo, nullptr, &m_depthImageView));
	m_debugUtils->SetObjectName(m_depthImageView, "DepthImageView");
}

void Renderer::CleanupDepthImage()
{
	vmaDestroyImage(m_allocator, m_depthImage, m_depthImageAllocation);
	m_disp->destroyImageView(m_depthImageView, nullptr);
}
