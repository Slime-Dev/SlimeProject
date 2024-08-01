#pragma once

#include <string>
#include <unordered_map>
#include <list>
#include <vulkan/vulkan_core.h>
#include "Model.h"
#include "PipelineGenerator.h"
#include <Light.h>
#include <type_traits>
#include <glm/fwd.hpp>
#include <glm/gtx/hash.hpp>
#include <imgui.h>
#include <vk_mem_alloc.h>
#include "ModelManager.h"

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

	void SetUp(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, VulkanDebugUtils& debugUtils);
	void CleanUp(vkb::DispatchTable& disp, VmaAllocator allocator);

	int Draw(vkb::DispatchTable& disp,
	        VkCommandBuffer& cmd,
	        ModelManager& modelManager,
	        DescriptorManager& descriptorManager,
	        VmaAllocator allocator,
	        VkCommandPool commandPool,
	        VkQueue graphicsQueue,
	        VulkanDebugUtils& debugUtils,
	        vkb::Swapchain swapchain,
	        std::vector<VkImage>& swapchainImages,
	        std::vector<VkImageView>& swapchainImageViews,
	        uint32_t imageIndex,
	        Scene* scene);

	void SetupViewportAndScissor(vkb::Swapchain swapchain, vkb::DispatchTable disp, VkCommandBuffer& cmd);
	void DrawModelsForShadowMap(vkb::DispatchTable disp, VulkanDebugUtils& debugUtils, VkCommandBuffer& cmd, ModelManager& modelManager, Scene* scene);
	void DrawModels(vkb::DispatchTable& disp, VkCommandBuffer& cmd, ModelManager& modelManager, DescriptorManager& descriptorManager, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, VulkanDebugUtils& debugUtils, Scene* scene);

	void DrawImguiDebugger(vkb::DispatchTable& disp, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, ModelManager& modelManager, VulkanDebugUtils& debugUtils);

	void CreateDepthImage(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, VulkanDebugUtils& debugUtils);

private:

	// Push Constant
	struct MVP
	{
		glm::mat4 model;
		glm::mat3 normalMatrix;
	} m_mvp;

	bool m_forceInvalidateDecriptorSets = false;

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

	VkDescriptorSet GetOrUpdateDescriptorSet(EntityManager& entityManager, Entity* entity, PipelineConfig* pipelineConfig, DescriptorManager& descriptorManager, VmaAllocator allocator, VulkanDebugUtils& debugUtils, int setIndex);
	void UpdateBasicMaterialDescriptors(EntityManager& entityManager, DescriptorManager& descriptorManager, VkDescriptorSet materialSet, Entity* entity, VmaAllocator allocator, int setIndex);
	void UpdatePBRMaterialDescriptors(EntityManager& entityManager, DescriptorManager& descriptorManager, VkDescriptorSet descSet, Entity* entity, VmaAllocator allocator, int setIndex);

	//
	/// SHADOWS ///////////////////////////////////
	//
	void GenerateShadowMap(vkb::DispatchTable& disp, VkCommandBuffer& cmd, ModelManager& modelManager, DescriptorManager& descriptorManager, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, VulkanDebugUtils& debugUtils, Scene* scene);
	void CreateShadowMap(vkb::DispatchTable& disp, VmaAllocator allocator, VulkanDebugUtils& debugUtils);
	void CleanUpShadowMap(vkb::DispatchTable& disp, VmaAllocator allocator);
	float GetShadowMapPixelValue(vkb::DispatchTable& disp, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, ModelManager& modelManager, int x, int y);
	void RenderShadowMapInspector(vkb::DispatchTable& disp, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, ModelManager& modelManager, VulkanDebugUtils& debugUtils);
	
	TextureResource m_shadowMap;
	ImTextureID m_shadowMapId;
	float m_shadowMapZoom = 1.0f;
	VkBuffer m_shadowMapStagingBuffer = VK_NULL_HANDLE;
	VmaAllocation m_shadowMapStagingBufferAllocation = VK_NULL_HANDLE;
	VkDeviceSize m_shadowMapStagingBufferSize = 0;
	unsigned int m_shadowMapWidth = 1080;
	unsigned int m_shadowMapHeight = 1080;
	unsigned int m_newShadowMapWidth = m_shadowMapWidth;
	unsigned int m_newShadowMapHeight = m_shadowMapHeight;

	//
	/// DEPTH TESTING ///////////////////////////////////
	//
	void CleanupDepthImage(vkb::DispatchTable& disp, VmaAllocator allocator);
	VkImage m_depthImage = VK_NULL_HANDLE;
	VkImageView m_depthImageView = VK_NULL_HANDLE;
	VmaAllocation m_depthImageAllocation;

	size_t GenerateDescriptorHash(const Entity* entity, int setIndex);
};
