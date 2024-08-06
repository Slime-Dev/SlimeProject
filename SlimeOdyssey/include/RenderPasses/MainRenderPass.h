#pragma once

#include "RenderPassBase.h"
#include "ShadowRenderPass.h"

class MainRenderPass : public RenderPassBase
{
public:
	MainRenderPass(ShadowRenderPass* shadowPass);

	void Execute(vkb::DispatchTable& disp, VkCommandBuffer& cmd, Scene* scene, Camera* camera) override;

private:
	void BindDescriptorSets(vkb::DispatchTable& disp, VkCommandBuffer& cmd, VkPipelineLayout layout, Scene* scene);

	VkDescriptorSet GetShadowMapDescriptorSet(Scene* scene, ShadowSystem& shadowSystem);

	ShadowRenderPass* m_shadowPass = nullptr;

	// Inherited via RenderPassBase
	void Setup(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, ShaderManager* shaderManager, VulkanDebugUtils& debugUtils) override;
	void Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator) override;
	VkRenderingInfo GetRenderingInfo(vkb::Swapchain swapchain, VkImageView& swapchainImageView, VkImageView& depthImageView) override;
};
