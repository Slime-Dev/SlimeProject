#include "MaterialManager.h"

#include <spdlog/spdlog.h>

#include "ResourcePathManager.h"
#include "VulkanUtil.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

MaterialManager::MaterialManager(vkb::DispatchTable& disp, VmaAllocator allocator, DescriptorManager* descriptorManager, VkQueue graphicsQueue, VkCommandPool commandPool)
      : m_disp(disp), m_allocator(allocator), m_descriptorManager(descriptorManager), m_graphicsQueue(graphicsQueue), m_commandPool(commandPool)
{
}

MaterialManager::~MaterialManager()
{
	// Destroy textures
	for (const auto& [textureName, texture]: m_textures)
	{
		if (texture)
		{
			if (texture->imageView != VK_NULL_HANDLE)
			{
				m_disp.destroyImageView(texture->imageView, nullptr);
			}
			if (texture->image != VK_NULL_HANDLE)
			{
				vmaDestroyImage(m_allocator, texture->image, texture->allocation);
			}
			if (texture->sampler != VK_NULL_HANDLE)
			{
				m_disp.destroySampler(texture->sampler, nullptr);
			}
		}
	}
	m_textures.clear();

	// Free descriptor sets (descriptor manager destroys the pool and all sets in it at once)
	m_descriptorSets.clear();

	// Destroy material buffers
	for (auto& [material, weakPtr]: m_materials)
	{
		if (auto sharedPtr = weakPtr.lock())
		{
			if (sharedPtr->configBuffer != VK_NULL_HANDLE)
			{
				vmaDestroyBuffer(m_allocator, sharedPtr->configBuffer, sharedPtr->configAllocation);
				sharedPtr->configBuffer = VK_NULL_HANDLE;
				sharedPtr->configAllocation = VK_NULL_HANDLE;
			}
		}
	}
	m_materials.clear();
}

std::shared_ptr<PBRMaterialResource> MaterialManager::CreatePBRMaterial()
{
	auto material = std::make_shared<PBRMaterialResource>();
	CreateMaterialBuffer(material.get(), sizeof(PBRMaterialResource::Config));
	m_materials[material.get()] = material;
	material->dirty = true;
	return material;
}

std::shared_ptr<BasicMaterialResource> MaterialManager::CreateBasicMaterial()
{
	auto material = std::make_shared<BasicMaterialResource>();
	CreateMaterialBuffer(material.get(), sizeof(BasicMaterialResource::Config));
	m_materials[material.get()] = material;
	material->dirty = true;
	return material;
}

void MaterialManager::CreateMaterialBuffer(MaterialResource* material, size_t configSize)
{
	if (material->configBuffer != VK_NULL_HANDLE)
	{
		vmaDestroyBuffer(m_allocator, material->configBuffer, material->configAllocation);
		material->configBuffer = VK_NULL_HANDLE;
		material->configAllocation = VK_NULL_HANDLE;
	}

	SlimeUtil::CreateBuffer("Material Config Buffer", m_allocator, configSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, material->configBuffer, material->configAllocation);
}

void MaterialManager::UpdateMaterialBuffer(MaterialResource* material)
{
	if (material->dirty)
	{
		if (auto pbrMaterial = dynamic_cast<PBRMaterialResource*>(material))
		{
			SlimeUtil::CopyStructToBuffer(pbrMaterial->config, m_allocator, material->configAllocation);
		}
		else if (auto basicMaterial = dynamic_cast<BasicMaterialResource*>(material))
		{
			SlimeUtil::CopyStructToBuffer(basicMaterial->config, m_allocator, material->configAllocation);
		}
		material->dirty = false;
	}
}

VkDescriptorSet MaterialManager::GetOrCreateDescriptorSet(MaterialResource* material, VkDescriptorSetLayout layout, TextureResource* shadowMap)
{
	auto it = m_descriptorSets.find(material);
	if (it != m_descriptorSets.end())
	{
		return it->second;
	}

	VkDescriptorSet newDescriptorSet = m_descriptorManager->AllocateDescriptorSet(layout);
	m_descriptorSets[material] = newDescriptorSet;

	// If it's a PBR material, bind the textures
	if (auto pbrMaterial = dynamic_cast<PBRMaterialResource*>(material))
	{
		SlimeUtil::BindBuffer(m_disp, newDescriptorSet, 0, material->configBuffer, 0, sizeof(PBRMaterialResource::Config));

		SlimeUtil::BindImage(m_disp, newDescriptorSet, 1, shadowMap->imageView, shadowMap->sampler);

		if (pbrMaterial->albedoTex)
			SlimeUtil::BindImage(m_disp, newDescriptorSet, 2, pbrMaterial->albedoTex->imageView, pbrMaterial->albedoTex->sampler);
		if (pbrMaterial->normalTex)
			SlimeUtil::BindImage(m_disp, newDescriptorSet, 3, pbrMaterial->normalTex->imageView, pbrMaterial->normalTex->sampler);
		if (pbrMaterial->metallicTex)
			SlimeUtil::BindImage(m_disp, newDescriptorSet, 4, pbrMaterial->metallicTex->imageView, pbrMaterial->metallicTex->sampler);
		if (pbrMaterial->roughnessTex)
			SlimeUtil::BindImage(m_disp, newDescriptorSet, 5, pbrMaterial->roughnessTex->imageView, pbrMaterial->roughnessTex->sampler);
		if (pbrMaterial->aoTex)
			SlimeUtil::BindImage(m_disp, newDescriptorSet, 6, pbrMaterial->aoTex->imageView, pbrMaterial->aoTex->sampler);
	}
	else
	{ // Assume basic material
		SlimeUtil::BindBuffer(m_disp, newDescriptorSet, 0, material->configBuffer, 0, sizeof(BasicMaterialResource::Config));
	}

	return newDescriptorSet;
}

void MaterialManager::CleanupUnusedMaterials()
{
	for (auto it = m_materials.begin(); it != m_materials.end();)
	{
		if (it->second.expired())
		{
			if (m_descriptorSets.count(it->first))
			{
				m_descriptorManager->FreeDescriptorSet(m_descriptorSets[it->first]);
				m_descriptorSets.erase(it->first);
			}

			if (it->first->configBuffer != VK_NULL_HANDLE)
			{
				vmaDestroyBuffer(m_allocator, it->first->configBuffer, it->first->configAllocation);
			}

			it = m_materials.erase(it);
		}
		else
		{
			++it;
		}
	}
}

std::shared_ptr<TextureResource> MaterialManager::LoadTexture(const std::string& name)
{
	std::string fullPath = ResourcePathManager::GetTexturePath(name);

	if (m_textures.contains(name))
	{
		spdlog::warn("Texture already exists: {}", name);
		return m_textures[name];
	}

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(fullPath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		spdlog::error("Failed to load texture '{}': {}", name, stbi_failure_reason());
		return nullptr;
	}

	VkDeviceSize imageSize = texWidth * texHeight * 4;

	TextureResource texture;
	texture.width = texWidth;
	texture.height = texHeight;

	// Create staging buffer
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	SlimeUtil::CreateBuffer("Load Texture Staging Buffer", m_allocator, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer, stagingAllocation);

	// Copy pixel data to staging buffer
	void* data;
	vmaMapMemory(m_allocator, stagingAllocation, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vmaUnmapMemory(m_allocator, stagingAllocation);

	stbi_image_free(pixels);

	// Create image
	SlimeUtil::CreateImage(name.c_str(), m_allocator, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY, texture.image, texture.allocation);

	// Transition image layout and copy buffer to image
	SlimeUtil::TransitionImageLayout(m_disp, m_graphicsQueue, m_commandPool, texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	SlimeUtil::CopyBufferToImage(m_disp, m_graphicsQueue, m_commandPool, stagingBuffer, texture.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	SlimeUtil::TransitionImageLayout(m_disp, m_graphicsQueue, m_commandPool, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Create image view
	texture.imageView = SlimeUtil::CreateImageView(m_disp, texture.image, VK_FORMAT_R8G8B8A8_SRGB);

	// Create sampler
	texture.sampler = SlimeUtil::CreateSampler(m_disp);

	// Cleanup staging buffer
	vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);

	m_textures[name] = std::make_shared<TextureResource>(texture);

	spdlog::debug("Texture '{}' loaded successfully", name);
	return m_textures[name];
}

const std::shared_ptr<TextureResource> MaterialManager::GetTexture(const std::string& name) const
{
	auto it = m_textures.find(name);
	if (it != m_textures.end())
	{
		return it->second;
	}
	return nullptr;
}

void MaterialManager::SetAllTextures(std::shared_ptr<PBRMaterialResource> material, std::string albedo, std::string normal, std::string metallic, std::string roughness, std::string ao)
{
	material->albedoTex = LoadTexture(albedo);
	material->normalTex = LoadTexture(normal);
	material->metallicTex = LoadTexture(metallic);
	material->roughnessTex = LoadTexture(roughness);
	material->aoTex = LoadTexture(ao);
}
