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
	}
}

std::pair<VkDescriptorSet, VkDescriptorSetLayout> DescriptorManager::GetSharedDescriptorSet()
{
	return m_sharedDescriptorSet;
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
	if (m_disp.allocateDescriptorSets(&allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor set");
	}

	m_descriptorSets[layoutIndex] = descriptorSet;

	// TODO: DO THIS BETTER I PROBABLY DONT WANT TO BE GRABBING THE FIRST RANDOM DESCRIPTOR AND HOPPING ITS CORRECT
	// If this is the shared descriptor set, store it (layout index 0 and shared descriptor set isnt allocated yet)
	if (layoutIndex == 0 && m_sharedDescriptorSet.first == VK_NULL_HANDLE)
	{
		m_sharedDescriptorSet = { descriptorSet, m_descriptorSetLayouts[layoutIndex] };
	}

	return descriptorSet;
}

VkDescriptorSet DescriptorManager::AllocateDescriptorSet(VkDescriptorSetLayout descriptorLayout)
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorLayout;

	VkDescriptorSet descriptorSet;
	if (m_disp.allocateDescriptorSets(&allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor set");
	}

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

	m_disp.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
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
	if (m_disp.createSampler(&samplerInfo, nullptr, &sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture sampler");
	}

	return sampler;
}

void DescriptorManager::DestroySampler(VkSampler sampler)
{
	// Destroy the sampler
	m_disp.destroySampler(sampler, nullptr);
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

	m_disp.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
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

	if (m_disp.createDescriptorPool(&poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool");
	}
}

std::shared_ptr<PBRMaterialResource> DescriptorManager::CreatePBRMaterial(VulkanContext& vulkanContext, ModelManager& modelManager, std::string name, std::string albedo, std::string normal, std::string metallic, std::string roughness, std::string ao)
{
	VkDevice device = vulkanContext.GetDevice();
	VkCommandPool commandPool = vulkanContext.GetCommandPool();
	VmaAllocator allocator = vulkanContext.GetAllocator();
	VkQueue graphicsQueue = vulkanContext.GetGraphicsQueue();
	vkb::DispatchTable disp = vulkanContext.GetDispatchTable();

	auto mat = std::make_shared<PBRMaterialResource>();

	SlimeUtil::CreateBuffer(name.c_str(), allocator, sizeof(PBRMaterialResource::Config), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, mat->configBuffer, mat->configAllocation);

	mat->albedoTex = modelManager.LoadTexture(disp, graphicsQueue, commandPool, allocator, this, albedo);
	mat->normalTex = modelManager.LoadTexture(disp, graphicsQueue, commandPool, allocator, this, normal);
	mat->metallicTex = modelManager.LoadTexture(disp, graphicsQueue, commandPool, allocator, this, metallic);
	mat->roughnessTex = modelManager.LoadTexture(disp, graphicsQueue, commandPool, allocator, this, roughness);
	mat->aoTex = modelManager.LoadTexture(disp, graphicsQueue, commandPool, allocator, this, ao);

	mat->disposed = false;

	return mat;
}

std::shared_ptr<BasicMaterialResource> DescriptorManager::CreateBasicMaterial(VulkanContext& vulkanContext, ModelManager& modelManager, std::string name)
{
	VkDevice device = vulkanContext.GetDevice();
	VkCommandPool commandPool = vulkanContext.GetCommandPool();
	VmaAllocator allocator = vulkanContext.GetAllocator();
	VkQueue graphicsQueue = vulkanContext.GetGraphicsQueue();
	vkb::DispatchTable disp = vulkanContext.GetDispatchTable();

	auto mat = std::make_shared<BasicMaterialResource>();

	SlimeUtil::CreateBuffer(name.c_str(), allocator, sizeof(BasicMaterialResource::Config), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, mat->configBuffer, mat->configAllocation);

	mat->disposed = false;

	return mat;
}
