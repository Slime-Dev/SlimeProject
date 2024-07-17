#pragma once

#include <ResourcePathManager.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "Model.h"
#include "PipelineGenerator.h"
#include "tiny_obj_loader.h"
#include "vk_mem_alloc.h"

class DescriptorManager;
class Engine;
class ResourcePathManager;
struct VmaAllocator_T;
using VmaAllocator = VmaAllocator_T*;

class ModelManager
{
public:
	ModelManager() = default;
	explicit ModelManager(ResourcePathManager& pathManager);
	~ModelManager();

	ModelResource* LoadModel(VmaAllocator allocator, const std::string& name, const std::string& pipelineName);
	bool LoadTexture(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VmaAllocator allocator, DescriptorManager* descriptorManager, const std::string& name);
	const TextureResource* GetTexture(const std::string& name) const;
	void UnloadAllResources(VkDevice device, VmaAllocator allocator);
	void BindTexture(VkDevice device, const std::string& name, uint32_t binding, VkDescriptorSet set);
	void TransitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
	int DrawModel(VkCommandBuffer& cmd, const ModelResource& model);
	void CreateImage(VmaAllocator allocator, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VkImage& image, VmaAllocation& allocation);

	std::map<std::string, PipelineContainer>& GetPipelines();

	void AddModel(const std::string& name, Model* model);
	void AddModelMap(const std::unordered_map<std::string, Model*>& models);
	
	// This will create a new model if it doesn't exist
	Model* GetModel(const std::string& name);

	// Iterator for models
	std::unordered_map<std::string, Model*>::iterator begin()
	{
		return m_models.begin();
	}

	std::unordered_map<std::string, Model*>::iterator end()
	{
		return m_models.end();
	}

	std::unordered_map<std::string, Model*>::const_iterator begin() const
	{
		return m_models.begin();
	}

	std::unordered_map<std::string, Model*>::const_iterator end() const
	{
		return m_models.end();
	}

private:
	ResourcePathManager m_pathManager;

	std::unordered_map<std::string, Model*> m_models;
	std::unordered_map<std::string, ModelResource> m_modelResources;
	std::unordered_map<std::string, TextureResource> m_textures;
	std::map<std::string, PipelineContainer> m_pipelines;

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
