#pragma once

#include "RenderPassBase.h"

class DescriptorManager;
class ModelManager;
class ShadowRenderPass;
class ShadowSystem;

class MainRenderPass : public RenderPassBase
{
public:
	MainRenderPass(ShadowRenderPass* shadowPass, ModelManager& modelManager, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, DescriptorManager& descriptorManager);

	void Execute(vkb::DispatchTable& disp, VkCommandBuffer& cmd, Scene* scene, Camera* camera) override;

private:
	void BindDescriptorSets(vkb::DispatchTable& disp, VkCommandBuffer& cmd, VkPipelineLayout layout, Scene* scene);

	VkDescriptorSet GetShadowMapDescriptorSet(Scene* scene, ShadowSystem& shadowSystem);

	ShadowRenderPass* m_shadowPass = nullptr;

	int DrawModel(vkb::DispatchTable& disp, VkCommandBuffer& cmd, const ModelResource& model);
	void Setup(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, ShaderManager* shaderManager, VulkanDebugUtils& debugUtils) override;
	void Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator) override;
	VkRenderingInfo GetRenderingInfo(vkb::Swapchain swapchain, VkImageView& swapchainImageView, VkImageView& depthImageView) override;
	
	VkPipeline m_pipeline;
	VkPipelineLayout m_pipelineLayout;
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;

	glm::vec3 m_clearColor = glm::vec3(0.0f, 0.0f, 0.0f);

	VkRenderingAttachmentInfo m_colorAttachmentInfo{};
	VkRenderingAttachmentInfo m_depthAttachmentInfo{};
	VkRenderingInfo m_renderingInfo{};

	ModelManager& m_modelManager;
	DescriptorManager& m_descriptorManager;
	VmaAllocator m_allocator;
	VkCommandPool m_commandPool;
	VkQueue m_graphicsQueue;
	VulkanDebugUtils m_debugUtils;
};
