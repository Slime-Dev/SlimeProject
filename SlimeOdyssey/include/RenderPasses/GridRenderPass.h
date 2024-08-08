#pragma once

#include <spdlog/spdlog.h>
#include "RenderPassBase.h"
#include "VulkanDebugUtils.h"

class ShaderManager;
class Camera;

class GridRenderPass : public RenderPassBase
{
public:
	GridRenderPass();
	void Execute(vkb::DispatchTable& disp, VkCommandBuffer& cmd, vkb::Swapchain swapchain, Scene* scene, Camera* camera) override;

private:
	VkPipeline m_pipeline;
	VkPipelineLayout m_pipelineLayout;

	// Inherited via RenderPassBase
	void Setup(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, ShaderManager* shaderManager, VulkanDebugUtils& debugUtils) override;
	void Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator) override;
	VkRenderingInfo* GetRenderingInfo(vkb::Swapchain swapchain, VkImageView& swapchainImageView, VkImageView& depthImageView) override;

	glm::vec3 m_clearColor = glm::vec3(0.0f, 0.0f, 0.0f);

	VkRenderingAttachmentInfo m_colorAttachmentInfo{};
	VkRenderingAttachmentInfo m_depthAttachmentInfo{};
	VkRenderingInfo m_renderingInfo{};
};
