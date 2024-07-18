//
// Created by alexm on 3/07/24.
//

#include "VulkanContext.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <string>

#define VMA_VULKAN_VERSION 1003000 // Vulkan 1.3
#define VMA_IMPLEMENTATION

#include <vk_mem_alloc.h>

#include "ModelManager.h"
#include "PipelineGenerator.h"
#include "VulkanUtil.h"

#include "Scene.h"

#define MAX_FRAMES_IN_FLIGHT 2

VulkanContext::~VulkanContext()
{
	if (!m_cleanUpFinished)
		spdlog::error("CLEANUP WAS NOT CALLED ON THE VULKAN CONTEXT!");
}

int VulkanContext::CreateContext(SlimeWindow* window)
{
	if (DeviceInit(window) != 0)
		return -1;
	if (CreateCommandPool() != 0)
		return -1;
	if (GetQueues() != 0)
		return -1;
	if (CreateSwapchain(window) != 0)
		return -1;
	if (CreateRenderCommandBuffers() != 0)
		return -1;
	if (InitSyncObjects() != 0)
		return -1;

	return 0;
}

int VulkanContext::DeviceInit(SlimeWindow* window)
{
	spdlog::info("Initializing Vulkan...");

	// set up the debug messenger to use spdlog
	auto debugCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) -> VkBool32
	{
		switch (messageSeverity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				spdlog::debug("{}", pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				spdlog::info("{}", pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				spdlog::warn("{}\n", pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				spdlog::error("{}\n", pCallbackData->pMessage);
				break;
			default:
				spdlog::error("Unknown message severity: {}", pCallbackData->pMessage);
		}

		return VK_FALSE;
	};

	// Create instance with VK_KHR_dynamic_rendering extension
	spdlog::info("Creating Vulkan instance...");
	vkb::InstanceBuilder instance_builder;
	auto instance_ret = instance_builder.request_validation_layers(true).set_debug_messenger_severity(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT).set_debug_callback(debugCallback).require_api_version(1, 3, 0).build();

	if (!instance_ret)
	{
		spdlog::error("Failed to create instance: {}", instance_ret.error().message());
		return -1;
	}
	spdlog::info("Vulkan instance created.");

	m_instance = instance_ret.value();
	m_instDisp = m_instance.make_table();

	// Create the window surface
	if (VkResult err = glfwCreateWindowSurface(m_instance.instance, window->GetGLFWWindow(), nullptr, &m_surface))
	{
		const char* error_msg;
		int ret = glfwGetError(&error_msg);
		if (ret != 0)
		{
			spdlog::error("GLFW error: {}", error_msg);
		}

		throw std::runtime_error("Failed to create window surface");
	}

	// Select physical device //
	spdlog::info("Selecting physical device...");
	VkPhysicalDeviceVulkan13Features features13 = {};
	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.dynamicRendering = VK_TRUE;
	features13.synchronization2 = VK_TRUE;

	VkPhysicalDeviceVulkan12Features features12 = {};
	features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features12.bufferDeviceAddress = VK_TRUE;
	features12.descriptorIndexing = VK_TRUE;

	VkPhysicalDeviceFeatures features = {};
	features.fillModeNonSolid = VK_TRUE;
	features.wideLines = VK_TRUE;

	vkb::PhysicalDeviceSelector phys_device_selector(m_instance);

	auto phys_device_ret = phys_device_selector.set_minimum_version(1, 3).set_required_features_13(features13).set_required_features_12(features12).set_required_features(features).set_surface(m_surface).select();

	if (!phys_device_ret)
	{
		spdlog::error("Failed to select physical device: {}", phys_device_ret.error().message());
		return -1;
	}
	spdlog::info("Physical device selected.");
	spdlog::info("Physical device: {}", phys_device_ret.value().properties.deviceName);

	vkb::PhysicalDevice physical_device = phys_device_ret.value();

	// Create device (extension is automatically enabled as it was required in PhysicalDeviceSelector)
	spdlog::info("Creating logical device...");
	vkb::DeviceBuilder device_builder{ physical_device };
	auto device_ret = device_builder.build();

	if (!device_ret)
	{
		spdlog::error("Failed to create logical device: {}", device_ret.error().message());
		return -1;
	}
	spdlog::info("Logical device created.");
	m_device = device_ret.value();

	m_disp = m_device.make_table();

	// Setup the VMA allocator
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = physical_device.physical_device;
	allocatorInfo.device = m_device.device;
	allocatorInfo.instance = m_instance.instance;

	if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create VMA allocator");
	}

	m_debugUtils = VulkanDebugUtils(m_instance, m_device);

	m_debugUtils.SetObjectName(m_device.device, "MainDevice");

	return 0;
}

int VulkanContext::CreateSwapchain(SlimeWindow* window)
{
	spdlog::info("Creating swapchain...");
	vkDeviceWaitIdle(m_device);

	vkb::SwapchainBuilder swapchain_builder(m_device, m_surface);

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
		m_disp.destroyImageView(image_view, nullptr);
	}

	m_swapchainImages = m_swapchain.get_images().value();
	m_swapchainImageViews = m_swapchain.get_image_views().value();

	for (size_t i = 0; i < m_swapchainImages.size(); i++)
	{
		m_debugUtils.SetObjectName(m_swapchainImages[i], "SwapchainImage_" + std::to_string(i));
		m_debugUtils.SetObjectName(m_swapchainImageViews[i], "SwapchainImageView_" + std::to_string(i));
	}

	// Clean up old depth image and image view
	if (m_depthImage)
	{
		vmaDestroyImage(m_allocator, m_depthImage, m_depthImageAllocation);
		m_disp.destroyImageView(m_depthImageView, nullptr);
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

	VkResult result = vmaCreateImage(m_allocator, &depthImageInfo, &depthAllocInfo, &m_depthImage, &m_depthImageAllocation, nullptr);
	if (result != VK_SUCCESS)
	{
		spdlog::error("Failed to create depth image!");
	}

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

	result = m_disp.createImageView(&depthImageViewInfo, nullptr, &m_depthImageView);
	if (result != VK_SUCCESS)
	{
		spdlog::error("Failed to create depth image View!");
	}

	return 0;
}

int VulkanContext::GetQueues()
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

	m_debugUtils.SetObjectName(m_graphicsQueue, "GraphicsQueue");
	m_debugUtils.SetObjectName(m_presentQueue, "PresentQueue");
	return 0;
}

int VulkanContext::CreateCommandPool()
{
	spdlog::info("Creating command pool...");
	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = m_device.get_queue_index(vkb::QueueType::graphics).value();
	// Assuming graphics queue family
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (m_disp.createCommandPool(&pool_info, nullptr, &m_commandPool) != VK_SUCCESS)
	{
		spdlog::error("Failed to create command pool!");
		return -1;
	}

	return 0;
}

int VulkanContext::CreateRenderCommandBuffers()
{
	spdlog::info("Recording command buffers...");
	m_renderCommandBuffers.resize(m_swapchain.image_count);

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = m_commandPool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = static_cast<uint32_t>(m_renderCommandBuffers.size());

	if (m_disp.allocateCommandBuffers(&alloc_info, m_renderCommandBuffers.data()) != VK_SUCCESS)
	{
		spdlog::error("Failed to allocate command buffers!");
		return -1;
	}

	int index = 0;
	for (auto& cmd: m_renderCommandBuffers)
	{
		m_debugUtils.SetObjectName(cmd, std::string("Render Command Buffer: " + std::to_string(index++)));
	}

	return 0;
}

int VulkanContext::Draw(VkCommandBuffer& cmd, int imageIndex, ModelManager& modelManager, DescriptorManager& descriptorManager, Scene* scene)
{
	if (SlimeUtil::BeginCommandBuffer(m_disp, cmd) != 0)
		return -1;

	m_renderer.SetupViewportAndScissor(m_swapchain, m_disp, cmd);
	SlimeUtil::SetupDepthTestingAndLineWidth(m_disp, cmd);

	// Transition color image to color attachment optimal
	modelManager.TransitionImageLayout(m_device, m_graphicsQueue, m_commandPool, m_swapchainImages[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	// Transition depth image to depth attachment optimal
	modelManager.TransitionImageLayout(m_device, m_graphicsQueue, m_commandPool, m_depthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkRenderingAttachmentInfo colorAttachmentInfo = {};
	colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachmentInfo.imageView = m_swapchainImageViews[imageIndex];
	colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentInfo.clearValue = { .color = { { 0.05f, 0.05f, 0.05f, 1.0f } } };

	VkRenderingAttachmentInfo depthAttachmentInfo = {};
	depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthAttachmentInfo.imageView = m_depthImageView;
	depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachmentInfo.clearValue.depthStencil.depth = 1.f;

	VkRenderingInfo renderingInfo = {};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.renderArea = {
		.offset = {0, 0},
          .extent = m_swapchain.extent
	};
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachmentInfo;
	renderingInfo.pDepthAttachment = &depthAttachmentInfo;

	m_disp.cmdBeginRendering(cmd, &renderingInfo);

	if (scene)
	{
		scene->Render(*this, modelManager);
		m_renderer.DrawModels(m_disp, m_debugUtils, m_allocator, cmd, modelManager, descriptorManager, scene);
	}

	m_disp.cmdEndRendering(cmd);

	// Transition color image to present src layout
	modelManager.TransitionImageLayout(m_device, m_graphicsQueue, m_commandPool, m_swapchainImages[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	return SlimeUtil::EndCommandBuffer(m_disp, cmd);
}

int VulkanContext::RenderFrame(ModelManager& modelManager, DescriptorManager& descriptorManager, SlimeWindow* window, Scene* scene)
{
	if (window->WindowSuspended())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		return 0;
	}

	// Wait for the frame to be finished
	if (m_disp.waitForFences(1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX) != VK_SUCCESS)
	{
		spdlog::error("Failed to wait for fence!");
		return -1;
	}

	uint32_t image_index;
	VkResult result = m_disp.acquireNextImageKHR(m_swapchain, UINT64_MAX, m_availableSemaphores[m_currentFrame], VK_NULL_HANDLE, &image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		if (CreateSwapchain(window) != 0)
			return -1;
		return 0;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		spdlog::error("Failed to acquire swapchain image!");
		return -1;
	}

	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (m_imageInFlight[image_index] != VK_NULL_HANDLE)
	{
		m_disp.waitForFences(1, &m_imageInFlight[image_index], VK_TRUE, UINT64_MAX);
	}

	// Mark the image as now being in use by this frame
	m_imageInFlight[image_index] = m_inFlightFences[m_currentFrame];

	// Begin command buffer recording
	VkCommandBuffer cmd = m_renderCommandBuffers[image_index];

	if (Draw(cmd, image_index, modelManager, descriptorManager, scene) != 0)
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

	m_disp.resetFences(1, &m_inFlightFences[m_currentFrame]);

	m_debugUtils.BeginQueueDebugMarker(m_graphicsQueue, "FrameSubmission", debugUtil_FrameSubmission);
	if (m_disp.queueSubmit(m_graphicsQueue, 1, &submit_info, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
	{
		spdlog::error("Failed to submit draw command buffer!");
		return -1;
	}
	m_debugUtils.EndQueueDebugMarker(m_graphicsQueue);

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphores;

	VkSwapchainKHR swapchains[] = { m_swapchain };
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swapchains;
	present_info.pImageIndices = &image_index;

	result = m_disp.queuePresentKHR(m_presentQueue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		if (CreateSwapchain(window) != 0)
			return -1;
	}
	else if (result != VK_SUCCESS)
	{
		spdlog::error("Failed to present swapchain image!");
		return -1;
	}

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	if (window->ShouldClose())
	{
		vkDeviceWaitIdle(m_device);
	}

	return 0;
}

int VulkanContext::InitSyncObjects()
{
	spdlog::info("Initializing synchronization objects...");
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
		if (m_disp.createSemaphore(&semaphore_info, nullptr, &m_availableSemaphores[i]) != VK_SUCCESS || m_disp.createSemaphore(&semaphore_info, nullptr, &m_finishedSemaphore[i]) != VK_SUCCESS || m_disp.createFence(&fence_info, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
		{
			spdlog::error("Failed to create synchronization objects!");
			return -1;
		}
	}

	return 0;
}

int VulkanContext::Cleanup(ShaderManager& shaderManager, ModelManager& modelManager, DescriptorManager& descriptorManager)
{
	spdlog::info("Cleaning up...");

	vkDeviceWaitIdle(m_device);

	// Destroy synchronization objects
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_disp.destroySemaphore(m_availableSemaphores[i], nullptr);
		m_disp.destroySemaphore(m_finishedSemaphore[i], nullptr);
		m_disp.destroyFence(m_inFlightFences[i], nullptr);
	}

	for (auto& image_view: m_swapchainImageViews)
	{
		m_disp.destroyImageView(image_view, nullptr);
	}

	modelManager.UnloadAllResources(m_device, m_allocator);

	shaderManager.CleanupDescriptorSetLayouts(m_device);

	descriptorManager.Cleanup();

	m_renderer.CleanUp(m_allocator);

	// Clean up old depth image and image view
	vmaDestroyImage(m_allocator, m_depthImage, m_depthImageAllocation);
	m_disp.destroyImageView(m_depthImageView, nullptr);

	shaderManager.CleanupShaderModules(m_device);

	m_disp.destroyCommandPool(m_commandPool, nullptr);

	vkb::destroy_swapchain(m_swapchain);
	vkb::destroy_surface(m_instance, m_surface);

	// Destroy the allocator
	vmaDestroyAllocator(m_allocator);

	vkb::destroy_device(m_device);
	vkb::destroy_instance(m_instance);

	m_cleanUpFinished = true;

	return 0;
}

// GETTERS //

VulkanDebugUtils& VulkanContext::GetDebugUtils()
{
	return m_debugUtils;
}

VkDevice VulkanContext::GetDevice() const
{
	return m_device.device;
}

const vkb::Swapchain& VulkanContext::GetSwapchain() const
{
	return m_swapchain;
}

VkQueue VulkanContext::GetGraphicsQueue() const
{
	return m_graphicsQueue;
}

VkQueue VulkanContext::GetPresentQueue() const
{
	return m_presentQueue;
}

VkCommandPool VulkanContext::GetCommandPool() const
{
	return m_commandPool;
}

VmaAllocator VulkanContext::GetAllocator() const
{
	return m_allocator;
}

const vkb::DispatchTable& VulkanContext::GetDispatchTable()
{
	return m_disp;
}
