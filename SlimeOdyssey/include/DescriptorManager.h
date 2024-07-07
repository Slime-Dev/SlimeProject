#pragma once

#include "glm/mat4x4.hpp"
#include <vector>
#include <unordered_map>
#include <vulkan/vulkan.h>

struct MVP
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

class DescriptorManager
{
public:
	DescriptorManager() = default;
	~DescriptorManager() = default;

	explicit DescriptorManager(VkDevice device);

	VkDescriptorSet AllocateDescriptorSet(uint32_t layoutIndex);
	VkDescriptorSet GetDescriptorSet(uint32_t layoutIndex);

	size_t AddDescriptorSetLayout(VkDescriptorSetLayout layout);
	size_t AddDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);

	void BindBuffer(VkDescriptorSet descriptorSet, uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

	void BindImage(VkDescriptorSet descriptorSet, uint32_t binding, VkImageView imageView, VkSampler sampler);

	void Cleanup();

private:
	void CreateDescriptorPool();

	VkDevice m_device;

	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	std::unordered_map<uint32_t, VkDescriptorSet> m_descriptorSets;

	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
};