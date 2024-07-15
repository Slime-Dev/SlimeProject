#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "vk_mem_alloc.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec3 tangent;
	glm::vec3 bitangent;

	bool operator==(const Vertex& other) const
	{
		return pos == other.pos && texCoord == other.texCoord && normal == other.normal;
	}
};

struct ModelResource
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	VkBuffer vertexBuffer;
	VmaAllocation vertexAllocation;

	VkBuffer indexBuffer;
	VmaAllocation indexAllocation;

	std::string pipeLineName;
};

struct TextureResource
{
	VmaAllocation allocation;

	VkImage image;
	VkImageView imageView;
	VkSampler sampler;

	uint32_t width;
	uint32_t height;
};

struct Material
{
	struct Config
	{
		glm::vec4 albedo = glm::vec4(1.0f); // Colour
		float metallic = 1.0f;              // Strength
		float roughness = 1.0f;             // Strength
		float ao = 1.0f;                    // Strength
	};

	const TextureResource* albedoTex;
	const TextureResource* normalTex;
	const TextureResource* metallicTex;
	const TextureResource* roughnessTex;
	const TextureResource* aoTex;

	Config config;
	VmaAllocation configAllocation;
	VkBuffer configBuffer;
};

struct Model
{
	ModelResource* model;
	glm::mat4 modelMat = glm::mat4(1.0f);
	Material material;
	bool isActive = true;
};