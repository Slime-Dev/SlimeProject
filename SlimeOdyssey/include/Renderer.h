#pragma once

#include <string>
#include <unordered_map>
#include <list>
#include <vulkan/vulkan_core.h>
#include "Model.h"
#include "PipelineGenerator.h"
#include <Light.h>

// Forward declarations
class Camera;
class DescriptorManager;
class Entity;
class EntityManager;
class Model;
class ModelManager;
class Scene;
class VulkanContext;
class VulkanDebugUtils;

namespace vkb
{
	struct DispatchTable;
	struct Swapchain;
} // namespace vkb

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

	template<>
	struct hash<DirectionalLight>
	{
		size_t operator()(const DirectionalLight& light) const
		{
			size_t h1 = hash<glm::vec3>()(light.direction);
			size_t h2 = hash<glm::vec3>()(light.color);
			size_t h3 = hash<float>()(light.ambientStrength);
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};
} // namespace std

class Renderer
{
public:
	Renderer() = default;
	~Renderer() = default;

	void SetupViewportAndScissor(vkb::Swapchain swapchain, vkb::DispatchTable disp, VkCommandBuffer& cmd);
	void DrawModelsForShadowMap(vkb::DispatchTable disp, VulkanDebugUtils& debugUtils, VkCommandBuffer& cmd, ModelManager& modelManager, Scene* scene);
	void DrawModels(vkb::DispatchTable disp, VulkanDebugUtils& debugUtils, VmaAllocator allocator, VkCommandBuffer& cmd, ModelManager& modelManager, DescriptorManager& descriptorManager, Scene* scene, TextureResource* shadowMap);

private:

	// Push Constant
	struct MVP
	{
		glm::mat4 model;
		glm::mat3 normalMatrix;
	} m_mvp;


	void UpdateCommonBuffers(VulkanDebugUtils& debugUtils, VmaAllocator allocator, VkCommandBuffer& cmd, Scene* scene);
	void UpdateLightBuffer(EntityManager& entityManager, VmaAllocator allocator);
	void UpdateCameraBuffer(EntityManager& entityManager, VmaAllocator allocator);
	PipelineConfig* BindPipeline(vkb::DispatchTable& disp, VkCommandBuffer& cmd, ModelManager& modelManager, const std::string& pipelineName, VulkanDebugUtils& debugUtils);
	void UpdatePushConstants(vkb::DispatchTable& disp, VkCommandBuffer& cmd, PipelineConfig& pipelineConfig, Transform& transform, VulkanDebugUtils& debugUtils);
	void DrawInfiniteGrid(vkb::DispatchTable& disp, VkCommandBuffer commandBuffer, const Camera& camera, VkPipeline gridPipeline, VkPipelineLayout gridPipelineLayout);

	void UpdateSharedDescriptors(DescriptorManager& descriptorManager, VkDescriptorSet sharedSet, VkDescriptorSetLayout setLayout, EntityManager& entityManager, VmaAllocator allocator);
	
	//
	/// MATERIALS ///////////////////////////////////
	//
	struct LRUCacheEntry
	{
		size_t hash;
		VkDescriptorSet descriptorSet;
	};

	std::list<LRUCacheEntry> m_lruList;
	std::unordered_map<size_t, std::list<LRUCacheEntry>::iterator> m_materialDescriptorCache;
	const size_t MAX_CACHE_SIZE = 75;

	VkDescriptorSet GetOrUpdateDescriptorSet(EntityManager& entityManager, Entity* entity, PipelineConfig* pipelineConfig, DescriptorManager& descriptorManager, VmaAllocator allocator, VulkanDebugUtils& debugUtils, TextureResource* shadowMap, int setIndex);
	void UpdateBasicMaterialDescriptors(EntityManager& entityManager, DescriptorManager& descriptorManager, VkDescriptorSet materialSet, Entity* entity, VmaAllocator allocator, int setIndex);
	void UpdatePBRMaterialDescriptors(EntityManager& entityManager, DescriptorManager& descriptorManager, VkDescriptorSet descSet, Entity* entity, VmaAllocator allocator, TextureResource* shadowMap, int setIndex);

	size_t GenerateDescriptorHash(const Entity* entity, int setIndex);
};
