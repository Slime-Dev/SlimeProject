#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#include "DescriptorManager.h"
#include "Material.h"
#include "vk_mem_alloc.h"

class MaterialManager
{
public:
	MaterialManager(vkb::DispatchTable& disp, VmaAllocator allocator, DescriptorManager* descriptorManager, VkCommandPool commandPool);
	~MaterialManager();

	void SetGraphicsQueue(VkQueue queue);

	std::shared_ptr<PBRMaterialResource> CreatePBRMaterial();
	std::shared_ptr<BasicMaterialResource> CreateBasicMaterial();

	void UpdateMaterialBuffer(MaterialResource* material);
	VkDescriptorSet GetOrCreateDescriptorSet(MaterialResource* material, VkDescriptorSetLayout layout, TextureResource* shadowMap);

	std::shared_ptr<TextureResource> LoadTexture(const std::string& name);
	const std::shared_ptr<TextureResource> GetTexture(const std::string& name) const;

	void SetAllTextures(std::shared_ptr<PBRMaterialResource> material, std::string albedo, std::string normal, std::string metallic, std::string roughness, std::string ao);

	void CleanupUnusedMaterials();

private:
	vkb::DispatchTable& m_disp;
	VmaAllocator m_allocator;
	DescriptorManager* m_descriptorManager;
	VkQueue m_graphicsQueue;
	VkCommandPool m_commandPool;

	std::unordered_map<MaterialResource*, std::weak_ptr<MaterialResource>> m_materials;
	std::unordered_map<MaterialResource*, VkDescriptorSet> m_descriptorSets;
	std::unordered_map<std::string, std::shared_ptr<TextureResource>> m_textures;

	void CreateMaterialBuffer(MaterialResource* material, size_t configSize);
};
