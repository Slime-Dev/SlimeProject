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
	void BindDescriptorSets(vkb::DispatchTable& disp, VkCommandBuffer& cmd, PipelineContainer& pipelineContainer, DescriptorManager& descriptorManager, EntityManager& entityManager, Entity* entity, VmaAllocator allocator);
	void BindCommonDescriptors(EntityManager& entityManager, DescriptorManager& descriptorManager, VkDescriptorSet descSet, VkDescriptorSetLayout layout, VmaAllocator allocator);
	void BindPBRMaterialDescriptors(DescriptorManager& descriptorManager, VkDescriptorSet descSet, VkDescriptorSetLayout layout, PBRMaterial& material, VmaAllocator allocator);
	void UpdatePushConstants(vkb::DispatchTable& disp, VkCommandBuffer& cmd, PipelineContainer& pipelineContainer, Transform& transform, VulkanDebugUtils& debugUtils);
	void DrawInfiniteGrid(vkb::DispatchTable& disp, VkCommandBuffer commandBuffer, const Camera& camera, VkPipeline gridPipeline, VkPipelineLayout gridPipelineLayout);
	
	void UpdateSharedDescriptors(DescriptorManager& descriptorManager, VkDescriptorSet sharedSet, VkDescriptorSetLayout setLayout, EntityManager& entityManager, VmaAllocator allocator);
	void UpdateMaterialDescriptors(DescriptorManager& descriptorManager, VkDescriptorSet materialSet, VkDescriptorSetLayout materialSetLayout, Entity* entity, VmaAllocator allocator);

	ShaderDebug m_shaderDebug;
	VkBuffer m_shaderDebugBuffer = VK_NULL_HANDLE;
	VmaAllocation m_shaderDebugAllocation = VK_NULL_HANDLE;
};
