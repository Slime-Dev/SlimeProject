#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Model.h"
#include "PipelineGenerator.h"
#include "tiny_obj_loader.h"

class DescriptorManager;
class VulkanContext;
struct VmaAllocator_T;
namespace vkb { struct DispatchTable; }
using VmaAllocator = VmaAllocator_T*;

class ModelManager
{
public:
	ModelManager() = default;
	~ModelManager();

	ModelResource* LoadModel(const std::string& name, const std::string& pipelineName);

	ModelResource* CreatePlane(VmaAllocator allocator, float size, int divisions);
	ModelResource* CreateLinePlane(VmaAllocator allocator);
	ModelResource* CreateCube(VmaAllocator allocator, float size = 1.0f);
	ModelResource* CreateSphere(VmaAllocator allocator, float radius = 1.0f, int segments = 16, int rings = 16);
	ModelResource* CreateCylinder(VmaAllocator allocator, float radius = 0.5f, float height = 2.0f, int segments = 16);

	void CreateShadowMapPipeline(VulkanContext& vulkanContext, ShaderManager& shaderManager, DescriptorManager& descriptorManager);
	void CreatePipeline(const std::string& pipelineName, VulkanContext& vulkanContext, ShaderManager& shaderManager, DescriptorManager& descriptorManager, const std::string& vertShaderPath, const std::string& fragShaderPath, bool depthTestEnabled, VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT, VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL);

	TextureResource* LoadTexture(vkb::DispatchTable& disp, VkQueue graphicsQueue, VkCommandPool commandPool, VmaAllocator allocator, DescriptorManager* descriptorManager, const std::string& name);
	const TextureResource* GetTexture(const std::string& name) const;
	void UnloadAllResources(vkb::DispatchTable& disp, VmaAllocator allocator);
	void BindTexture(vkb::DispatchTable& disp, const std::string& name, uint32_t binding, VkDescriptorSet set);
	void TransitionImageLayout(vkb::DispatchTable& disp, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
	int DrawModel(vkb::DispatchTable& disp, VkCommandBuffer& cmd, const ModelResource& model);
	void CreateBuffersForMesh(VmaAllocator allocator, ModelResource& model);
	TextureResource* CopyTexture(const std::string& name, TextureResource* texture);

	std::map<std::string, PipelineConfig>& GetPipelines();

	void CleanUpAllPipelines(vkb::DispatchTable& disp);

private:
	std::unordered_map<std::string, ModelResource> m_modelResources;
	std::unordered_map<std::string, TextureResource> m_textures;
	std::map<std::string, PipelineConfig> m_pipelines;

	void CenterModel(std::vector<Vertex>& vector);
	void CalculateTexCoords(std::vector<Vertex>& vector, const std::vector<unsigned int>& indices);
	int GetDominantAxis(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
	void CalculateProjectedTexCoords(Vertex& v0, Vertex& v1, Vertex& v2);
	glm::vec3 CalculateTangent(const glm::vec3& edge1, const glm::vec3& edge2, const glm::vec2& deltaUV1, const glm::vec2& deltaUV2, float f);
	glm::vec3 CalculateBitangent(const glm::vec3& edge1, const glm::vec3& edge2, const glm::vec2& deltaUV1, const glm::vec2& deltaUV2, float f);
	void AssignTexCoords(Vertex& v0, Vertex& v1, Vertex& v2, const glm::vec3& tangent, const glm::vec3& bitangent);
	bool LoadObjFile(std::string& fullPath, tinyobj::attrib_t& attrib, std::vector<tinyobj::shape_t>& shapes, std::vector<tinyobj::material_t>& materials, std::string& warn, std::string& err);
	void ProcessVerticesAndIndices(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes, ModelResource& model);
	Vertex CreateVertex(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index);
	void AddUniqueVertex(const Vertex& vertex, ModelResource& model, std::unordered_map<Vertex, uint32_t>& uniqueVertices);
	void CalculateNormals(ModelResource& model);
	void CalculateTangentsAndBitangents(ModelResource& model);
	void CalculateTangentSpace(Vertex& v0, Vertex& v1, Vertex& v2);
	glm::vec3 ExtractPosition(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index);
	glm::vec2 ExtractTexCoord(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index);
	glm::vec3 ExtractNormal(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index);
	void CalculateFaceNormals(const ModelResource& model, std::vector<glm::vec3>& faceNormals, std::vector<std::vector<uint32_t>>& vertexFaces);
	void AverageVertexNormals(ModelResource& model, const std::vector<glm::vec3>& faceNormals, const std::vector<std::vector<uint32_t>>& vertexFaces);
	VkImageView CreateImageView(vkb::DispatchTable& disp, VkImage image, VkFormat format);
	void CopyBufferToImage(vkb::DispatchTable& disp, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
};

template<>
struct std::hash<Vertex>
{
	size_t operator()(Vertex const& vertex) const
	{
		return ((std::hash<glm::vec3>()(vertex.pos) ^ (std::hash<glm::vec2>()(vertex.texCoord) << 1)) >> 1) ^ (std::hash<glm::vec3>()(vertex.normal) << 1);
	}
};
