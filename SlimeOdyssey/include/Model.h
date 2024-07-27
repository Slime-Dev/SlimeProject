#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Component.h"
#include "vk_mem_alloc.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <memory>

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

struct MaterialResource
{
	VmaAllocation configAllocation;
	VkBuffer configBuffer;

	bool disposed = false;
};

struct BasicMaterialResource : public MaterialResource
{
	struct Config
	{
		glm::vec4 albedo = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f); // Colour
	};

	Config config;
};

struct PBRMaterialResource : public MaterialResource
{
	struct Config
	{
		glm::vec4 albedo = glm::vec4(1.0f); // Colour
		float metallic = 0.5f;              // Strength
		float roughness = 0.5f;             // Strength
		float ao = 0.5f;                    // Strength
	};

	TextureResource* albedoTex;
	TextureResource* normalTex;
	TextureResource* metallicTex;
	TextureResource* roughnessTex;
	TextureResource* aoTex;

	Config config;
};

struct PBRMaterial : public Component
{
	PBRMaterial() = default;
	PBRMaterial(PBRMaterialResource* material)
	      : materialResource(material){};
	PBRMaterial(std::shared_ptr<PBRMaterialResource> material)
	      : materialResource(material.get()){};

	PBRMaterialResource* materialResource = nullptr;

	void ImGuiDebug();
};

struct BasicMaterial : public Component
{
	BasicMaterial() = default;
	BasicMaterial(BasicMaterialResource* material)
	      : materialResource(material){};
	BasicMaterial(std::shared_ptr<BasicMaterialResource> material)
	      : materialResource(material.get()){};

	BasicMaterialResource* materialResource = nullptr;

	void ImGuiDebug();
};

struct Model : public Component
{
	Model() = default;
	Model(ModelResource* model)
	      : modelResource(model){};
	ModelResource* modelResource;
	void ImGuiDebug();
};

struct Transform : public Component
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f);
	glm::vec3 scale = glm::vec3(1.0f);

	glm::mat4 GetModelMatrix() const
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, position);
		model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, scale);
		return model;
	}

	void ImGuiDebug();
};
