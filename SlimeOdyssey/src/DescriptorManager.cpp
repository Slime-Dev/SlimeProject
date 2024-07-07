#include "DescriptorManager.h"

#include <stdexcept>

DescriptorManager::DescriptorManager(VkDevice device): m_device(device)
{
	CreateDescriptorPool();
}

void DescriptorManager::Cleanup()
{
	if (m_descriptorPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
	}
}

VkDescriptorSet DescriptorManager::AllocateDescriptorSet(uint32_t layoutIndex)
{
	if (layoutIndex >= m_descriptorSetLayouts.size())
	{
		throw std::runtime_error("Invalid layout index");
	}

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool     = m_descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts        = &m_descriptorSetLayouts[layoutIndex];

	VkDescriptorSet descriptorSet;
	if (vkAllocateDescriptorSets(m_device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor set");
	}

	m_descriptorSets[layoutIndex] = descriptorSet;

	return descriptorSet;
}

VkDescriptorSet DescriptorManager::GetDescriptorSet(uint32_t layoutIndex)
{
	if (layoutIndex >= m_descriptorSetLayouts.size())
	{
		throw std::runtime_error("Invalid layout index");
	}

	return m_descriptorSets[layoutIndex];
}

size_t DescriptorManager::AddDescriptorSetLayout(VkDescriptorSetLayout layout)
{
	m_descriptorSetLayouts.push_back(layout);
	return m_descriptorSetLayouts.size() - 1;
}

size_t DescriptorManager::AddDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
{
	size_t startIndex = m_descriptorSetLayouts.size();
	m_descriptorSetLayouts.insert(m_descriptorSetLayouts.end(), descriptorSetLayouts.begin(), descriptorSetLayouts.end());
	return startIndex;
}

void DescriptorManager::BindBuffer(VkDescriptorSet descriptorSet, uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
{
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = buffer;
	bufferInfo.offset = offset;
	bufferInfo.range  = range;

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet          = descriptorSet;
	descriptorWrite.dstBinding      = binding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo     = &bufferInfo;

	vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
}

void DescriptorManager::BindImage(VkDescriptorSet descriptorSet, uint32_t binding, VkImageView imageView, VkSampler sampler)
{
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView   = imageView;
	imageInfo.sampler     = sampler;

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet          = descriptorSet;
	descriptorWrite.dstBinding      = binding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo      = &imageInfo;

	vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
}

void DescriptorManager::CreateDescriptorPool()
{
	VkDescriptorPoolSize poolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 }
	};

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes    = poolSizes;
	poolInfo.maxSets       = 100; // Adjust this based on your needs

	if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool");
	}
}