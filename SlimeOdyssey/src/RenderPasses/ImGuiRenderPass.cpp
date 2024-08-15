#include "RenderPasses/ImGuiRenderPass.h"

#include "Camera.h"
#include "PipelineGenerator.h"
#include "ResourcePathManager.h"
#include "Scene.h"
#include "ShaderManager.h"
#include <VkBootstrap.h>
#include "RenderPassManager.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

ImGuiRenderPass::ImGuiRenderPass()
{
	name = "ImGui";
}

void ImGuiRenderPass::Setup(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, ShaderManager* shaderManager, VulkanDebugUtils& debugUtils)
{
	// Set up rendering info for ImGui
	m_colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	m_colorAttachmentInfo.imageView = VK_NULL_HANDLE;
	m_colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	m_colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	m_colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	m_renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	m_renderingInfo.renderArea = {
		{0, 0},
       swapchain.extent
	};
	m_renderingInfo.layerCount = 1;
	m_renderingInfo.colorAttachmentCount = 1;
}

void ImGuiRenderPass::Execute(vkb::DispatchTable& disp, VkCommandBuffer& cmd, vkb::Swapchain swapchain, Scene* scene, Camera* camera, RenderPassManager* renderPassManager)
{
	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Create a full screen docking space for ImGui
	ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode;
	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID, ImGui::GetMainViewport(), dockFlags);

	// Render your ImGui windows and widgets here
	scene->Render();

	// Render the renderpasses imguis
	renderPassManager->DrawImGui(disp);

	ImGui::Render();
	ImDrawData* drawData = ImGui::GetDrawData();
	const bool isMinimized = (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f);
	if (!isMinimized)
	{
		// Record dear imgui primitives into command buffer
		ImGui_ImplVulkan_RenderDrawData(drawData, cmd);
	}
}

void ImGuiRenderPass::Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator)
{

}

VkRenderingInfo* ImGuiRenderPass::GetRenderingInfo(vkb::Swapchain swapchain, VkImageView& swapchainImageView, VkImageView& depthImageView)
{
	m_colorAttachmentInfo.imageView = swapchainImageView;
	m_renderingInfo.pColorAttachments = &m_colorAttachmentInfo;


	m_colorAttachmentInfo.imageView = swapchainImageView;

	m_renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	m_renderingInfo.pNext = VK_NULL_HANDLE;
	m_renderingInfo.flags = 0;
	m_renderingInfo.renderArea = {
		.offset = {0, 0},
          .extent = swapchain.extent
	};
	m_renderingInfo.layerCount = 1;
	m_renderingInfo.colorAttachmentCount = 1;
	m_renderingInfo.pColorAttachments = &m_colorAttachmentInfo;

	return &m_renderingInfo;
}
