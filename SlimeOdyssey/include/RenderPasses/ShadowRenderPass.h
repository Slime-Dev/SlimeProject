#pragma once

#include "RenderPassBase.h"
#include "ShadowSystem.h"

class ShadowRenderPass : public RenderPassBase
{
public:
	ShadowRenderPass(ModelManager& modelManager, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue);

	void Setup(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, ShaderManager* shaderManager, VulkanDebugUtils& debugUtils) override;

	void Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator) override;

	void Execute(vkb::DispatchTable& disp, VkCommandBuffer& cmd, Scene* scene, Camera* camera) override;

	VkRenderingInfo GetRenderingInfo(vkb::Swapchain swapchain, VkImageView& swapchainImageView, VkImageView& depthImageView) override;

	ShadowSystem& GetShadowSystem();

private:
	void DrawModelsForShadowMap(vkb::DispatchTable disp, VulkanDebugUtils& debugUtils, VkCommandBuffer& cmd, ModelManager& modelManager, Scene* scene);

	ShadowSystem m_shadowSystem;
	ModelManager& m_modelManager;
	VmaAllocator m_allocator;
	VkCommandPool m_commandPool;
	VkQueue m_graphicsQueue;
	VulkanDebugUtils m_debugUtils;
};
