#pragma once

#include <spdlog/spdlog.h>
#include "RenderPassBase.h"
#include "VulkanDebugUtils.h"

class ShaderManager;
class Camera;

class ImGuiRenderPass : public RenderPassBase
{
public:
	ImGuiRenderPass();
	void Execute(vkb::DispatchTable& disp, VkCommandBuffer& cmd, vkb::Swapchain swapchain, Scene* scene, Camera* camera, RenderPassManager* renderPassManager) override;

private:
	// Inherited via RenderPassBase
	void Setup(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, ShaderManager* shaderManager, VulkanDebugUtils& debugUtils) override;
	void Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator) override;
	VkRenderingInfo* GetRenderingInfo(vkb::Swapchain swapchain, VkImageView& swapchainImageView, VkImageView& depthImageView) override;

	VkRenderingAttachmentInfo m_colorAttachmentInfo{};
	VkRenderingInfo m_renderingInfo{};
};
