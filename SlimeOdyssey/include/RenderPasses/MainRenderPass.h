#pragma once

#include <list>
#include <unordered_map>
#include <type_traits>
#include <glm/gtx/hash.hpp>
#include "glm/fwd.hpp"
#include "RenderPassBase.h"
#include "Model.h"

class DescriptorManager;
class MaterialManager;
class ModelManager;
class ShadowRenderPass;
class ShadowSystem;
struct ModelResource;
struct Transform;

class MainRenderPass : public RenderPassBase
{
public:
	MainRenderPass(std::shared_ptr<ShadowRenderPass> shadowPass, MaterialManager* materialManager, ModelManager* modelManager, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, DescriptorManager* descriptorManager);

	void Execute(vkb::DispatchTable& disp, VkCommandBuffer& cmd, vkb::Swapchain swapchain, Scene* scene, Camera* camera, RenderPassManager* renderPassManager) override;
	void ImGuiDraw(vkb::DispatchTable disp) override;

private:
	// Push Constant
	struct MVP
	{
		glm::mat4 model;
		glm::mat3 normalMatrix;
	} m_mvp;

	std::shared_ptr<ShadowRenderPass> m_shadowPass = nullptr;

	int DrawModel(vkb::DispatchTable& disp, VkCommandBuffer& cmd, const ModelResource& model);
	void SetupViewportAndScissor(vkb::DispatchTable& disp, VkCommandBuffer& cmd, vkb::Swapchain swapchain);
	void Setup(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, ShaderManager* shaderManager, VulkanDebugUtils& debugUtils) override;
	void Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator) override;
	VkRenderingInfo* GetRenderingInfo(vkb::Swapchain swapchain, VkImageView& swapchainImageView, VkImageView& depthImageView) override;

	VkPipeline m_pipeline;
	VkPipelineLayout m_pipelineLayout;
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;

	glm::vec3 m_clearColor;

	VkRenderingAttachmentInfo m_colorAttachmentInfo{};
	VkRenderingAttachmentInfo m_depthAttachmentInfo{};
	VkRenderingInfo m_renderingInfo{};

	MaterialManager* m_materialManager = nullptr;
	ModelManager* m_modelManager = nullptr;
	DescriptorManager* m_descriptorManager = nullptr;
	VmaAllocator m_allocator;
	VkCommandPool m_commandPool;
	VkQueue m_graphicsQueue;
	VulkanDebugUtils* m_debugUtils = nullptr;

	void UpdateCommonBuffers(VkCommandBuffer& cmd, Scene* scene);
	void UpdateSharedDescriptors(vkb::DispatchTable& disp, VkDescriptorSet cameraSet, VkDescriptorSet lightSet, entt::registry& registry);
	void UpdatePushConstants(vkb::DispatchTable& disp, VkCommandBuffer& cmd, Transform& transform);

};
