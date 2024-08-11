#pragma once

#include "RenderPassBase.h"
#include "ShadowSystem.h"

class ShadowRenderPass : public RenderPassBase
{
public:
	ShadowRenderPass(ModelManager& modelManager, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue);

	void Setup(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, ShaderManager* shaderManager, VulkanDebugUtils& debugUtils) override;

	void Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator) override;

	void Execute(vkb::DispatchTable& disp, VkCommandBuffer& cmd, vkb::Swapchain swapchain, Scene* scene, Camera* camera, RenderPassManager* renderPassManager) override;

	VkRenderingInfo* GetRenderingInfo(vkb::Swapchain swapchain, VkImageView& swapchainImageView, VkImageView& depthImageView) override;

	ShadowSystem& GetShadowSystem();
	
	void ImGuiDraw(vkb::DispatchTable disp) override;

private:
	void DrawModelsForShadowMap(vkb::DispatchTable disp, VulkanDebugUtils& debugUtils, VkCommandBuffer& cmd, ModelManager& modelManager, Scene* scene);

	int DrawModel(vkb::DispatchTable& disp, VkCommandBuffer& cmd, const ModelResource& model);

	VkPipeline m_pipeline;
	VkPipelineLayout m_pipelineLayout;

	ShadowSystem m_shadowSystem;
	ModelManager& m_modelManager;
	VmaAllocator m_allocator;
	VkCommandPool m_commandPool;
	VkQueue m_graphicsQueue;
	VulkanDebugUtils m_debugUtils;
};
