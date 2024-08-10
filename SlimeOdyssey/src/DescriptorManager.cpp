#include "DescriptorManager.h"

#include <stdexcept>

#include "ModelManager.h"
#include "VulkanContext.h"
#include "VulkanUtil.h"

DescriptorManager::DescriptorManager(const vkb::DispatchTable& disp)
      : m_disp(disp)
{
	CreateDescriptorPool();
}

void DescriptorManager::Cleanup()
{
	if (m_descriptorPool != VK_NULL_HANDLE)
	{
		m_disp.destroyDescriptorPool(m_descriptorPool, nullptr);
		m_descriptorPool = VK_NULL_HANDLE;
		m_descriptorSetCount = 0;
	}
}

std::pair<VkDescriptorSet, VkDescriptorSetLayout> DescriptorManager::GetSharedDescriptorSet()
{
	return m_sharedDescriptorSet;
}

void DescriptorManager::CreateSharedDescriptorSet(VkDescriptorSetLayout descriptorsetLayout)
{
	m_sharedDescriptorSet = { AllocateDescriptorSet(descriptorsetLayout), descriptorsetLayout };
}

std::pair<VkDescriptorSet, VkDescriptorSetLayout> DescriptorManager::GetLightDescriptorSet()
{
	return m_lightDescriptorSet;
}

void DescriptorManager::CreateLightDescriptorSet(VkDescriptorSetLayout descriptorsetLayout)
{
	m_lightDescriptorSet = { AllocateDescriptorSet(descriptorsetLayout), descriptorsetLayout };
}

VkDescriptorSet DescriptorManager::AllocateDescriptorSet(VkDescriptorSetLayout descriptorLayout)
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorLayout;

	VkDescriptorSet descriptorSet;
	VK_CHECK(m_disp.allocateDescriptorSets(&allocInfo, &descriptorSet));
	m_descriptorSetCount++;

	return descriptorSet;
}

void DescriptorManager::FreeDescriptorSet(VkDescriptorSet descriptorSet)
{
	VK_CHECK(m_disp.freeDescriptorSets(m_descriptorPool, 1, &descriptorSet));
	m_descriptorSetCount--;
}

void DescriptorManager::CreateDescriptorPool()
{
	VkDescriptorPoolSize poolSizes[] = {
		{         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 }
	};

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = 100;

	VK_CHECK(m_disp.createDescriptorPool(&poolInfo, nullptr, &m_descriptorPool));
	m_descriptorSetCount = 0;

	spdlog::debug("Created command pool");
}
