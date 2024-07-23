#pragma once
#include <string>
#include <vulkan/vulkan_core.h>

#include "DescriptorManager.h"
#include "Entity.h"
#include "EntityManager.h"
#include "Model.h"
#include "ModelManager.h"
#include "PipelineGenerator.h"
#include "vk_mem_alloc.h"
#include <functional>

class VulkanDebugUtils;
class Camera;

namespace vkb
{
	struct DispatchTable;
	struct Swapchain;
} // namespace vkb

class Scene;
class VulkanContext;
class DescriptorManager;
class ModelManager;

#include <functional>

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
			size_t h1 = hash<glm::vec4>()(config.albedo);
			size_t h2 = hash<float>()(config.metallic);
			size_t h3 = hash<float>()(config.roughness);
			size_t h4 = hash<float>()(config.ao);
			return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
		}
	};
} // namespace std

class Renderer
{
public:
	Renderer() = default;
	~Renderer() = default;

	void SetupViewportAndScissor(vkb::Swapchain swapchain, vkb::DispatchTable disp, VkCommandBuffer& cmd);
	void DrawModels(vkb::DispatchTable disp, VulkanDebugUtils& debugUtils, VmaAllocator allocator, VkCommandBuffer& cmd, ModelManager& modelManager, DescriptorManager& descriptorManager, Scene* scene);
	void CleanUp(VmaAllocator allocator);

private:
	MVP m_mvp; // Push Constant

	void UpdateCommonBuffers(VulkanDebugUtils& debugUtils, VmaAllocator allocator, VkCommandBuffer& cmd, Scene* scene);
	void UpdateLightBuffer(EntityManager& entityManager, VmaAllocator allocator);
	void UpdateCameraBuffer(EntityManager& entityManager, VmaAllocator allocator);
	PipelineContainer* BindPipeline(vkb::DispatchTable& disp, VkCommandBuffer& cmd, ModelManager& modelManager, const std::string& pipelineName, VulkanDebugUtils& debugUtils);
	void UpdatePushConstants(vkb::DispatchTable& disp, VkCommandBuffer& cmd, PipelineContainer& pipelineContainer, Transform& transform, VulkanDebugUtils& debugUtils);
	void DrawInfiniteGrid(vkb::DispatchTable& disp, VkCommandBuffer commandBuffer, const Camera& camera, VkPipeline gridPipeline, VkPipelineLayout gridPipelineLayout);
	
	void UpdateSharedDescriptors(DescriptorManager& descriptorManager, VkDescriptorSet sharedSet, VkDescriptorSetLayout setLayout, EntityManager& entityManager, VmaAllocator allocator);

	VkDescriptorSet GetOrUpdateMaterialDescriptorSet(Entity* entity, PipelineContainer* pipelineContainer, DescriptorManager& descriptorManager, VmaAllocator allocator);

	void UpdateBasicMaterialDescriptors(DescriptorManager& descriptorManager, VkDescriptorSet materialSet, Entity* entity, VmaAllocator allocator);

	void UpdatePBRMaterialDescriptors(DescriptorManager& descriptorManager, VkDescriptorSet descSet, Entity* entity, VmaAllocator allocator);

	ShaderDebug m_shaderDebug;
	VkBuffer m_shaderDebugBuffer = VK_NULL_HANDLE;
	VmaAllocation m_shaderDebugAllocation = VK_NULL_HANDLE;

	    // Add this member to cache material descriptor sets
	std::unordered_map<size_t, VkDescriptorSet> m_materialDescriptorCache;

	// Helper function to generate a hash for a material
	size_t GenerateMaterialHash(const Entity* entity)
	{
		size_t hash = 0;
		if (entity->HasComponent<BasicMaterial>())
		{
			const auto& material = entity->GetComponent<BasicMaterial>().materialResource;
			hash = std::hash<BasicMaterialResource::Config>{}(material->config);
		}
		else if (entity->HasComponent<PBRMaterial>())
		{
			const auto& material = entity->GetComponent<PBRMaterial>().materialResource;
			hash = std::hash<PBRMaterialResource::Config>{}(material->config);
			// Include texture pointers in the hash
			hash ^= std::hash<const void*>{}(material->albedoTex);
			hash ^= std::hash<const void*>{}(material->normalTex);
			hash ^= std::hash<const void*>{}(material->metallicTex);
			hash ^= std::hash<const void*>{}(material->roughnessTex);
			hash ^= std::hash<const void*>{}(material->aoTex);
		}
		return hash;
	}
};
