#include "DescriptorManager.h"

#include <stdexcept>
#include "VulkanContext.h"
#include "VulkanUtil.h"
#include "ModelManager.h"

DescriptorManager::DescriptorManager(VkDevice device)
      : m_device(device)
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
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_descriptorSetLayouts[layoutIndex];

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
	bufferInfo.range = range;

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = binding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
}

VkSampler DescriptorManager::CreateSampler()
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	VkSampler sampler;
	if (vkCreateSampler(m_device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture sampler");
	}

	return sampler;
}

void DescriptorManager::DestroySampler(VkSampler sampler)
{
	// Destroy the sampler
	vkDestroySampler(m_device, sampler, nullptr);
}

void DescriptorManager::BindImage(VkDescriptorSet descriptorSet, uint32_t binding, VkImageView imageView, VkSampler sampler)
{
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = imageView;
	imageInfo.sampler = sampler;

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = binding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
}

void DescriptorManager::CreateDescriptorPool()
{
	VkDescriptorPoolSize poolSizes[] = {
		{         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 }
	};

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = 100; // Adjust this based on your needs

	if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool");
	}
}

Material DescriptorManager::CreateMaterial(VulkanContext& vulkanContext, ModelManager& modelManager, std::string name, std::string albedo, std::string normal, std::string metallic, std::string roughness, std::string ao)
{
	VkDevice device = vulkanContext.GetDevice();
	VkCommandPool commandPool = vulkanContext.GetCommandPool();
	VmaAllocator allocator = vulkanContext.GetAllocator();
	VkQueue graphicsQueue = vulkanContext.GetGraphicsQueue();

	Material::Config config;
	VkBuffer configBuffer;
	VmaAllocation configAllocation;
	
	SlimeUtil::CreateBuffer(name.c_str(), allocator, sizeof(Material::Config), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, configBuffer, configAllocation);

	const TextureResource* albedoTex = modelManager.LoadTexture(device, graphicsQueue, commandPool, allocator, this, albedo);
	const TextureResource* normalTex = modelManager.LoadTexture(device, graphicsQueue, commandPool, allocator, this, normal);
	const TextureResource* metallicTex = modelManager.LoadTexture(device, graphicsQueue, commandPool, allocator, this, metallic);
	const TextureResource* roughnessTex = modelManager.LoadTexture(device, graphicsQueue, commandPool, allocator, this, roughness);
	const TextureResource* aoTex = modelManager.LoadTexture(device, graphicsQueue, commandPool, allocator, this, ao);

	return {
		.albedoTex = albedoTex, 
		.normalTex = normalTex, 
		.metallicTex = metallicTex, 
		.roughnessTex = roughnessTex, 
		.aoTex = aoTex, 

		.config = config, .configAllocation = configAllocation,
		.configBuffer = configBuffer,

		.disposed = false
	};
}
