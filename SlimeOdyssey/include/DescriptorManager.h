#pragma once

#include <glm/mat4x4.hpp>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include "Model.h"
#include "VkBootstrapDispatch.h"
#include <memory>

// Forward declarations
struct MVP;
class VulkanContext;
class ModelManager;

// Structures
struct MVP
{
	glm::mat4 model;
	glm::mat3 normalMatrix;
};

struct ShaderDebug
{
	int debugMode = 0;     // 0: normal render, 1: show normals, 2: show light direction, 3: show view direction
	bool useNormalMap = true; // Toggle normal mapping
};

class DescriptorManager
{
public:
	// Constructors and Destructor
	DescriptorManager() = default;
	explicit DescriptorManager(const vkb::DispatchTable& disp);
	~DescriptorManager() = default;

	// Descriptor Set Management
	VkDescriptorSet AllocateDescriptorSet(uint32_t layoutIndex);
	VkDescriptorSet GetDescriptorSet(uint32_t layoutIndex);
	size_t AddDescriptorSetLayout(VkDescriptorSetLayout layout);
	size_t AddDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);

	// Resource Binding
	void BindBuffer(VkDescriptorSet descriptorSet, uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);
	void BindImage(VkDescriptorSet descriptorSet, uint32_t binding, VkImageView imageView, VkSampler sampler);

	// Sampler Management
	VkSampler CreateSampler();
	void DestroySampler(VkSampler sampler);

	// Cleanup
	void Cleanup();

	std::shared_ptr<MaterialResource> CreateMaterial(VulkanContext& vulkanContext, ModelManager& modelManager, std::string name, std::string albedo, std::string normal, std::string metallic, std::string roughness, std::string ao);

private:
	// Private Methods
	void CreateDescriptorPool();

	// Member Variables
	const vkb::DispatchTable& m_disp;

	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	std::unordered_map<uint32_t, VkDescriptorSet> m_descriptorSets;
};
