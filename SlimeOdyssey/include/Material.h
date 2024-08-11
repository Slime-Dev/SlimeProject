#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Component.h"
#include "vk_mem_alloc.h"

struct TextureResource
{
	VmaAllocation allocation = VK_NULL_HANDLE;
	VkImage image = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkSampler sampler = VK_NULL_HANDLE;
	uint32_t width = 0;
	uint32_t height = 0;
};

struct MaterialResource
{
	VmaAllocation configAllocation = VK_NULL_HANDLE;
	VkBuffer configBuffer = VK_NULL_HANDLE;
	bool dirty = false;
	virtual ~MaterialResource() = default;
};

struct BasicMaterialResource : public MaterialResource
{
	struct Config
	{
		glm::vec4 albedo = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
	};

	Config config;
};

struct PBRMaterialResource : public MaterialResource
{
	struct Config
	{
		glm::vec3 albedo = glm::vec3(1.0f);
		float metallic = 0.5f;
		float roughness = 0.5f;
		float ao = 0.5f;
		glm::vec2 padding = glm::vec2(420.0f);
	};

	Config config;
	std::shared_ptr<TextureResource> albedoTex;
	std::shared_ptr<TextureResource> normalTex;
	std::shared_ptr<TextureResource> metallicTex;
	std::shared_ptr<TextureResource> roughnessTex;
	std::shared_ptr<TextureResource> aoTex;
};

struct PBRMaterial : public Component
{
	PBRMaterial() = default;

	PBRMaterial(std::shared_ptr<PBRMaterialResource> material)
	      : materialResource(material)
	{
	}

	std::shared_ptr<PBRMaterialResource> materialResource;

	void ImGuiDebug();
};

struct BasicMaterial : public Component
{
	BasicMaterial() = default;

	BasicMaterial(std::shared_ptr<BasicMaterialResource> material)
	      : materialResource(material)
	{
	}

	std::shared_ptr<BasicMaterialResource> materialResource;

	void ImGuiDebug();
};
