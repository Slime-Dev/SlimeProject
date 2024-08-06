#include "Renderer.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <memory>
#include <vk_mem_alloc.h>

#include "DescriptorManager.h"
#include "ModelManager.h"
#include "RenderPasses/GridRenderPass.h"
#include "RenderPasses/MainRenderPass.h"
#include "RenderPasses/ShadowRenderPass.h"
#include "Scene.h"
#include "VulkanContext.h"
#include "VulkanUtil.h"

void Renderer::SetUp(vkb::DispatchTable* disp, VmaAllocator allocator, vkb::Swapchain swapchain, VulkanDebugUtils* debugUtils, ShaderManager* shaderManager)
{
	m_disp = disp;
	m_allocator = allocator;
	m_swapchain = swapchain;
	m_debugUtils = debugUtils;

	CreateDepthImage();
	SetupRenderPasses(shaderManager);
}

void Renderer::CleanUp()
{
	m_renderPassManager.Cleanup(*m_disp, m_allocator);
	CleanupDepthImage();
}

int Renderer::Draw(VkCommandBuffer& cmd, ModelManager& modelManager, DescriptorManager& descriptorManager, VkCommandPool commandPool, VkQueue graphicsQueue, std::vector<VkImage>& swapchainImages, std::vector<VkImageView>& swapchainImageViews, uint32_t imageIndex, Scene* scene)
{
	if (SlimeUtil::BeginCommandBuffer(*m_disp, cmd) != 0)
		return -1;

	Camera* camera = scene->m_entityManager.GetEntityByName("MainCamera")->GetComponentPtr<Camera>();

	SetupViewportAndScissor(cmd);
	SlimeUtil::SetupDepthTestingAndLineWidth(*m_disp, cmd);

	// Transition images to appropriate layouts
	TransitionImages(modelManager, graphicsQueue, commandPool, swapchainImages[imageIndex]);

	// Execute all render passes
	m_renderPassManager.ExecutePasses(*m_disp, cmd, *m_debugUtils, m_swapchain, swapchainImageViews[imageIndex], m_depthImageView, scene, camera);

	// Render ImGui
	RenderImGui(cmd, swapchainImages[imageIndex], swapchainImageViews[imageIndex]);

	// Transition color image to present src layout
	modelManager.TransitionImageLayout(*m_disp, graphicsQueue, commandPool, swapchainImages[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	if (SlimeUtil::EndCommandBuffer(*m_disp, cmd) != 0)
		return -1;

	// Handle multi-viewport rendering
	HandleMultiViewportRendering();

	return 0;
}

void Renderer::SetupRenderPasses(ShaderManager* shaderManager)
{
	auto shadowPass = new ShadowRenderPass(*m_modelManager, m_allocator, m_commandPool, m_graphicsQueue);
	//m_renderPassManager.AddPass(shadowPass);

	auto mainPass = std::make_shared<MainRenderPass>(shadowPass, *m_modelManager, m_allocator, m_commandPool, m_graphicsQueue, m_descriptorManager);
	m_renderPassManager.AddPass(mainPass);

	auto gridPass = std::make_shared<GridRenderPass>();
	m_renderPassManager.AddPass(gridPass);

	m_renderPassManager.Setup(*m_disp, m_allocator, m_swapchain, shaderManager, *m_debugUtils);
}

void Renderer::SetupViewportAndScissor(VkCommandBuffer& cmd)
{
	VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(m_swapchain.extent.width), static_cast<float>(m_swapchain.extent.height), 0.0f, 1.0f };
	VkRect2D scissor = {
		{ 0, 0 },
        m_swapchain.extent
	};

	m_disp->cmdSetViewport(cmd, 0, 1, &viewport);
	m_disp->cmdSetScissor(cmd, 0, 1, &scissor);
}

void Renderer::TransitionImages(ModelManager& modelManager, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage swapchainImage)
{
	// Transition color image to color attachment optimal
	modelManager.TransitionImageLayout(*m_disp, graphicsQueue, commandPool, swapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	// Transition depth image to depth attachment optimal
	modelManager.TransitionImageLayout(*m_disp, graphicsQueue, commandPool, m_depthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
}

void Renderer::RenderImGui(VkCommandBuffer& cmd, VkImage swapchainImage, VkImageView swapchainImageView)
{
	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Create a full screen docking space for ImGui
	ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode;
	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID, ImGui::GetMainViewport(), dockFlags);

	// Render your ImGui windows and widgets here
	RenderImGuiWindows();

	ImGui::Render();
	ImDrawData* drawData = ImGui::GetDrawData();
	const bool isMinimized = (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f);
	if (!isMinimized)
	{
		// Set up rendering info for ImGui
		VkRenderingAttachmentInfo colorAttachmentInfo = {};
		colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachmentInfo.imageView = swapchainImageView;
		colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		VkRenderingInfo renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea = {
			{ 0, 0 },
            m_swapchain.extent
		};
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachmentInfo;

		m_disp->cmdBeginRendering(cmd, &renderingInfo);

		// Record dear imgui primitives into command buffer
		ImGui_ImplVulkan_RenderDrawData(drawData, cmd);

		m_disp->cmdEndRendering(cmd);
	}
}

void Renderer::RenderImGuiWindows()
{
	// Render your ImGui windows here
	// For example:
	ImGui::Begin("Renderer Settings");
	ImGui::ColorEdit3("Clear Color", &m_clearColor.r);
	// Add more ImGui widgets as needed
	ImGui::End();

	// m_shadowSystem.RenderImGuiWindow();
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

	VK_CHECK(vmaCreateImage(m_allocator, &depthImageInfo, &depthAllocInfo, &m_depthImage, &m_depthImageAllocation, nullptr));
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
