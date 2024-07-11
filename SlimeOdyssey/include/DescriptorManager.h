#pragma once

#include <glm/mat4x4.hpp>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

// Forward declarations
struct MVP;
class DescriptorManager;

// Structures
struct MVP
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat3 normalMatrix;
};

class DescriptorManager
{
public:
	// Constructors and Destructor
	DescriptorManager() = default;
	explicit DescriptorManager(VkDevice device);
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

private:
	// Private Methods
	void CreateDescriptorPool();

	// Member Variables
	VkDevice m_device;
	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	std::unordered_map<uint32_t, VkDescriptorSet> m_descriptorSets;
};
