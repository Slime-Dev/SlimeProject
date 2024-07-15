#pragma once

#include <ResourcePathManager.h>
#include <string>
#include <unordered_map>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include "tiny_obj_loader.h"
#include "vk_mem_alloc.h"

class DescriptorManager;
class Engine;
class ResourcePathManager;
struct VmaAllocator_T;
using VmaAllocator = VmaAllocator_T*;

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
	glm::mat4 model = glm::mat4(1.0f);
	std::string pipeLineName;
	bool isActive = true;
};

struct TextureResource
{
	VkImage image;
	VkSampler sampler;
	VmaAllocation allocation;
	VkImageView imageView;
	uint32_t width;
	uint32_t height;
};

struct TempMaterialTextures
{
	const TextureResource* albedo;
	const TextureResource* normal;
	const TextureResource* metallic;
	const TextureResource* roughness;
	const TextureResource* ao;
};

class ModelManager
{
public:
	ModelManager() = default;
	explicit ModelManager(ResourcePathManager& pathManager);
	~ModelManager() = default;

	ModelResource* LoadModel(VmaAllocator allocator, const std::string& name, const std::string& pipelineName);
	bool LoadTexture(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VmaAllocator allocator, DescriptorManager* descriptorManager, const std::string& name);
	const ModelResource* GetModel(const std::string& name) const;
	const TextureResource* GetTexture(const std::string& name) const;
	void UnloadResource(VkDevice device, VmaAllocator allocator, const std::string& name);
	void UnloadAllResources(VkDevice device, VmaAllocator allocator);
	void BindModel(const std::string& name, VkCommandBuffer commandBuffer);
	void BindTexture(VkDevice device, const std::string& name, uint32_t binding, VkDescriptorSet set);
	void TransitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
	int DrawModel(VkCommandBuffer& cmd, const std::string& name);
	int DrawModel(VkCommandBuffer& cmd, const ModelResource& model);
	void CreateImage(VmaAllocator allocator, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VkImage& image, VmaAllocation& allocation);

	// Iterator for models
	std::unordered_map<std::string, ModelResource>::iterator begin()
	{
		return m_models.begin();
	}

	std::unordered_map<std::string, ModelResource>::iterator end()
	{
		return m_models.end();
	}

	std::unordered_map<std::string, ModelResource>::const_iterator begin() const
	{
		return m_models.begin();
	}

	std::unordered_map<std::string, ModelResource>::const_iterator end() const
	{
		return m_models.end();
	}

private:
	ResourcePathManager m_pathManager;

	std::unordered_map<std::string, ModelResource> m_models;
	std::unordered_map<std::string, TextureResource> m_textures;

	void CenterModel(std::vector<Vertex>& vector);
	void CalculateTexCoords(std::vector<Vertex>& vector, const std::vector<unsigned int>& indices);
	int GetDominantAxis(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
	void CalculateProjectedTexCoords(Vertex& v0, Vertex& v1, Vertex& v2);
	glm::vec3 CalculateTangent(const glm::vec3& edge1, const glm::vec3& edge2, const glm::vec2& deltaUV1, const glm::vec2& deltaUV2, float f);
	glm::vec3 CalculateBitangent(const glm::vec3& edge1, const glm::vec3& edge2, const glm::vec2& deltaUV1, const glm::vec2& deltaUV2, float f);
	void AssignTexCoords(Vertex& v0, Vertex& v1, Vertex& v2, const glm::vec3& tangent, const glm::vec3& bitangent);
	bool LoadObjFile(const std::string& fullPath, tinyobj::attrib_t& attrib, std::vector<tinyobj::shape_t>& shapes, std::vector<tinyobj::material_t>& materials, std::string& warn, std::string& err);
	void ProcessVerticesAndIndices(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes, ModelResource& model);
	Vertex CreateVertex(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index);
	void AddUniqueVertex(const Vertex& vertex, ModelResource& model, std::unordered_map<Vertex, uint32_t>& uniqueVertices);
	void CalculateNormals(ModelResource& model);
	void CalculateTangentsAndBitangents(ModelResource& model);
	void CalculateTangentSpace(Vertex& v0, Vertex& v1, Vertex& v2);
	void CreateBuffers(VmaAllocator allocator, ModelResource& model);
	glm::vec3 ExtractPosition(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index);
	glm::vec2 ExtractTexCoord(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index);
	glm::vec3 ExtractNormal(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index);
	void CalculateFaceNormals(const ModelResource& model, std::vector<glm::vec3>& faceNormals, std::vector<std::vector<uint32_t>>& vertexFaces);
	void AverageVertexNormals(ModelResource& model, const std::vector<glm::vec3>& faceNormals, const std::vector<std::vector<uint32_t>>& vertexFaces);
	VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format);
	void CopyBufferToImage(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
};

template<>
struct std::hash<Vertex>
{
	size_t operator()(Vertex const& vertex) const
	{
		return ((std::hash<glm::vec3>()(vertex.pos) ^ (std::hash<glm::vec2>()(vertex.texCoord) << 1)) >> 1) ^ (std::hash<glm::vec3>()(vertex.normal) << 1);
	}
};