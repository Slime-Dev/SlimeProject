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
	void FreeDescriptorSet(VkDescriptorSet descriptorSet);

	// Cleanup
	void Cleanup();

	std::pair<VkDescriptorSet, VkDescriptorSetLayout> GetSharedDescriptorSet();
	void CreateSharedDescriptorSet(VkDescriptorSetLayout descriptorsetLayout);
	
	std::pair<VkDescriptorSet, VkDescriptorSetLayout> GetLightDescriptorSet();
	void CreateLightDescriptorSet(VkDescriptorSetLayout descriptorsetLayout);

	int GetDescriptorSetCount() const
	{
		return m_descriptorSetCount;
	}

private:
	// Private Methods
	void CreateDescriptorPool();

	// Member Variables
	const vkb::DispatchTable& m_disp;

	std::pair<VkDescriptorSet, VkDescriptorSetLayout> m_sharedDescriptorSet;
	std::pair<VkDescriptorSet, VkDescriptorSetLayout> m_lightDescriptorSet;

	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
	int m_descriptorSetCount = 0;
};
