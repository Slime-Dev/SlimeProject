#pragma once

#include <glm/mat4x4.hpp>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include "Model.h"
#include "VkBootstrapDispatch.h"
#include <memory>

// Forward declarations
class VulkanContext;
class ModelManager;

class DescriptorManager
{
public:
	// Constructors and Destructor
	DescriptorManager() = default;
	explicit DescriptorManager(const vkb::DispatchTable& disp);
	~DescriptorManager() = default;

	// Descriptor Set Management
	VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout descriptorLayout);
	VkDescriptorSet GetDescriptorSet(uint32_t layoutIndex);
	size_t AddDescriptorSetLayout(VkDescriptorSetLayout layout);
	size_t AddDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);

	// Resource Binding
	void BindBuffer(VkDescriptorSet descriptorSet, uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);
	void BindImage(VkDescriptorSet descriptorSet, uint32_t binding, VkImageView imageView, VkSampler sampler);

	// Cleanup
	void Cleanup();

	std::pair<VkDescriptorSet, VkDescriptorSetLayout> GetSharedDescriptorSet();
	void CreateSharedDescriptorSet(VkDescriptorSetLayout descriptorsetLayout);

	std::shared_ptr<PBRMaterialResource> CreatePBRMaterial(VulkanContext& vulkanContext, ModelManager& modelManager, std::string name, std::string albedo, std::string normal, std::string metallic, std::string roughness, std::string ao);
	std::shared_ptr<BasicMaterialResource> CreateBasicMaterial(VulkanContext& vulkanContext, ModelManager& modelManager, std::string name);

	std::shared_ptr<PBRMaterialResource> CopyPBRMaterial(VulkanContext& vulkanContext, ModelManager& modelManager, std::string name, std::shared_ptr<PBRMaterialResource> inMaterial);
	std::shared_ptr<BasicMaterialResource> CopyBasicMaterial(VulkanContext& vulkanContext, ModelManager& modelManager, std::string name, std::shared_ptr<BasicMaterialResource> inMaterial);

private:
	// Private Methods
	void CreateDescriptorPool();

	// Member Variables
	const vkb::DispatchTable& m_disp;

	std::pair<VkDescriptorSet, VkDescriptorSetLayout> m_sharedDescriptorSet;

	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	std::unordered_map<uint32_t, VkDescriptorSet> m_descriptorSets;
};
