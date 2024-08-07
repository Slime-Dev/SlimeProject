#pragma once

#include <list>
#include <unordered_map>
#include <type_traits>
#include <glm/gtx/hash.hpp>
#include "glm/fwd.hpp"
#include "RenderPassBase.h"
#include "Model.h"

class DescriptorManager;
class ModelManager;
class ShadowRenderPass;
class ShadowSystem;
class Entity;
struct ModelResource;
struct Transform;

// Custom hash functions for material configurations
namespace std
{
	template<>
	struct hash<BasicMaterialResource::Config>
	{
		size_t operator()(const BasicMaterialResource::Config& config) const
		{
			return hash<glm::vec4>()(config.albedo);
		}
	};

	template<>
	struct hash<PBRMaterialResource::Config>
	{
		size_t operator()(const PBRMaterialResource::Config& config) const
		{
			size_t h1 = hash<glm::vec3>()(config.albedo);
			size_t h2 = hash<float>()(config.metallic);
			size_t h3 = hash<float>()(config.roughness);
			size_t h4 = hash<float>()(config.ao);
			return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
		}
	};
} // namespace std

class MainRenderPass : public RenderPassBase
{
public:
	MainRenderPass(ShadowRenderPass* shadowPass, ModelManager* modelManager, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, DescriptorManager* descriptorManager);

	void Execute(vkb::DispatchTable& disp, VkCommandBuffer& cmd, Scene* scene, Camera* camera) override;

private:
	// Push Constant
	struct MVP
	{
		glm::mat4 model;
		glm::mat3 normalMatrix;
	} m_mvp;

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

	ModelManager* m_modelManager = nullptr;
	DescriptorManager* m_descriptorManager = nullptr;
	VmaAllocator m_allocator;
	VkCommandPool m_commandPool;
	VkQueue m_graphicsQueue;
	VulkanDebugUtils* m_debugUtils = nullptr;

	void UpdateCommonBuffers(VkCommandBuffer& cmd, Scene* scene);

	void UpdateSharedDescriptors(VkDescriptorSet cameraSet, VkDescriptorSet lightSet, EntityManager& entityManager);

	struct LRUCacheEntry
	{
		size_t hash;
		VkDescriptorSet descriptorSet;
	};

	std::list<LRUCacheEntry> m_lruList;
	std::unordered_map<size_t, std::list<LRUCacheEntry>::iterator> m_materialDescriptorCache;
	const size_t MAX_CACHE_SIZE = 75;

	VkDescriptorSet GetOrUpdateDescriptorSet(EntityManager& entityManager, Entity* entity);
	void UpdatePBRMaterialDescriptors(EntityManager& entityManager, VkDescriptorSet descSet, Entity* entity);
	size_t GenerateDescriptorHash(const Entity* entity);
	void UpdatePushConstants(vkb::DispatchTable& disp, VkCommandBuffer& cmd, Transform& transform);
	bool m_forceInvalidateDecriptorSets = false;
};
