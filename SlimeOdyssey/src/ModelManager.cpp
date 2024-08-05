#include "ModelManager.h"

#include <spdlog/spdlog.h>
#include <stdexcept>
#include <vk_mem_alloc.h>

// Assuming we're using tinyobj for model loading
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// Assuming we're using stb_image for texture loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "ResourcePathManager.h"
#include "VulkanContext.h"
#include "VulkanUtil.h"
#include "ShaderManager.h"

ModelManager::~ModelManager()
{
	// Check for any remaining models or resources
	if (!m_modelResources.empty() || !m_textures.empty())
	{
		std::runtime_error("Model Manager not cleaned up correctly.");
	}
}

void ModelManager::CenterModel(std::vector<Vertex>& vector)
{
	glm::vec3 min = vector[0].pos;
	glm::vec3 max = vector[0].pos;

	for (const Vertex& vertex: vector)
	{
		min = glm::min(min, vertex.pos);
		max = glm::max(max, vertex.pos);
	}

	for (Vertex& vertex: vector)
	{
		vertex.pos -= (min + max) / 2.0f;
	}
}

void ModelManager::CalculateTexCoords(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
	if (indices.size() % 3 != 0)
	{
		throw std::invalid_argument("Indices must be a multiple of 3 for triangles");
	}

	for (size_t i = 0; i < indices.size(); i += 3)
	{
		Vertex& v0 = vertices[indices[i]];
		Vertex& v1 = vertices[indices[i + 1]];
		Vertex& v2 = vertices[indices[i + 2]];

		glm::vec3 edge1 = v1.pos - v0.pos;
		glm::vec3 edge2 = v2.pos - v0.pos;

		// Calculate texture space vectors
		glm::vec2 deltaUV1 = v1.texCoord - v0.texCoord;
		glm::vec2 deltaUV2 = v2.texCoord - v0.texCoord;

		float det = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
		if (std::abs(det) < 1e-6f)
		{
			// Handle degenerate UV case
			CalculateProjectedTexCoords(v0, v1, v2);
			continue;
		}

		float f = 1.0f / det;

		glm::vec3 tangent = CalculateTangent(edge1, edge2, deltaUV1, deltaUV2, f);
		glm::vec3 bitangent = CalculateBitangent(edge1, edge2, deltaUV1, deltaUV2, f);

		// Normalize the tangent and bitangent
		tangent = glm::normalize(tangent);
		bitangent = glm::normalize(bitangent);

		// Calculate new texture coordinates
		AssignTexCoords(v0, v1, v2, tangent, bitangent);
	}
}

void ModelManager::CalculateProjectedTexCoords(Vertex& v0, Vertex& v1, Vertex& v2)
{
	// Project onto the plane with the largest area
	int dominantAxis = GetDominantAxis(v0.pos, v1.pos, v2.pos);
	int u = (dominantAxis + 1) % 3;
	int v = (dominantAxis + 2) % 3;

	v0.texCoord = glm::vec2(v0.pos[u], v0.pos[v]);
	v1.texCoord = glm::vec2(v1.pos[u], v1.pos[v]);
	v2.texCoord = glm::vec2(v2.pos[u], v2.pos[v]);
}

int ModelManager::GetDominantAxis(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
{
	glm::vec3 normal = glm::abs(glm::cross(v1 - v0, v2 - v0));
	return (normal.x > normal.y && normal.x > normal.z) ? 0 : (normal.y > normal.z) ? 1 : 2;
}

glm::vec3 ModelManager::CalculateTangent(const glm::vec3& edge1, const glm::vec3& edge2, const glm::vec2& deltaUV1, const glm::vec2& deltaUV2, float f)
{
	return { f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x), f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y), f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z) };
}

glm::vec3 ModelManager::CalculateBitangent(const glm::vec3& edge1, const glm::vec3& edge2, const glm::vec2& deltaUV1, const glm::vec2& deltaUV2, float f)
{
	return { f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x), f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y), f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z) };
}

void ModelManager::AssignTexCoords(Vertex& v0, Vertex& v1, Vertex& v2, const glm::vec3& tangent, const glm::vec3& bitangent)
{
	v0.texCoord = glm::vec2(glm::dot(v0.pos, tangent), glm::dot(v0.pos, bitangent));
	v1.texCoord = glm::vec2(glm::dot(v1.pos, tangent), glm::dot(v1.pos, bitangent));
	v2.texCoord = glm::vec2(glm::dot(v2.pos, tangent), glm::dot(v2.pos, bitangent));
}

bool ModelManager::LoadObjFile(std::string& fullPath, tinyobj::attrib_t& attrib, std::vector<tinyobj::shape_t>& shapes, std::vector<tinyobj::material_t>& materials, std::string& warn, std::string& err)
{
	// First test the file path
	if (fullPath.empty())
	{
		spdlog::error("Model path is empty");
		return false;
	}

	// Test if we need to make the path lower case by using ifstream to check if the file exists
	std::ifstream file(fullPath.c_str());
	if (!file.good())
	{
		// Try to make the path lower case
		std::string lowerCasePath = fullPath;
		std::transform(lowerCasePath.begin(), lowerCasePath.end(), lowerCasePath.begin(), ::tolower);
		file.open(lowerCasePath.c_str());

		if (!file.good())
		{
			spdlog::error("Model file not found: {}", fullPath);
			return false;
		}

		fullPath = lowerCasePath;
	}

	file.close();

	// Load the obj
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fullPath.c_str()))
	{
		spdlog::error("Failed to load model '{}': {}", fullPath, err);
		return false;
	}
	return true;
}

void ModelManager::ProcessVerticesAndIndices(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes, ModelResource& model)
{
	std::unordered_map<Vertex, uint32_t> uniqueVertices;
	for (const auto& shape: shapes)
	{
		for (const auto& index: shape.mesh.indices)
		{
			Vertex vertex = CreateVertex(attrib, index);
			AddUniqueVertex(vertex, model, uniqueVertices);
		}
	}
}

Vertex ModelManager::CreateVertex(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index)
{
	Vertex vertex;
	vertex.pos = ExtractPosition(attrib, index);
	vertex.texCoord = ExtractTexCoord(attrib, index);
	vertex.normal = ExtractNormal(attrib, index);
	return vertex;
}

glm::vec3 ModelManager::ExtractPosition(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index)
{
	return { attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1], attrib.vertices[3 * index.vertex_index + 2] };
}

glm::vec2 ModelManager::ExtractTexCoord(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index)
{
	if (!attrib.texcoords.empty())
	{
		return { attrib.texcoords[2 * index.texcoord_index + 0], 1.0f - attrib.texcoords[2 * index.texcoord_index + 1] };
	}
	return { 0.0f, 0.0f };
}

glm::vec3 ModelManager::ExtractNormal(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index)
{
	if (!attrib.normals.empty())
	{
		return { attrib.normals[3 * index.normal_index + 0], attrib.normals[3 * index.normal_index + 1], attrib.normals[3 * index.normal_index + 2] };
	}
	return { 0.0f, 0.0f, 0.0f };
}

void ModelManager::AddUniqueVertex(const Vertex& vertex, ModelResource& model, std::unordered_map<Vertex, uint32_t>& uniqueVertices)
{
	if (uniqueVertices.count(vertex) == 0)
	{
		uniqueVertices[vertex] = static_cast<uint32_t>(model.vertices.size());
		model.vertices.push_back(vertex);
	}
	model.indices.push_back(uniqueVertices[vertex]);
}

void ModelManager::CalculateNormals(ModelResource& model)
{
	std::vector<glm::vec3> faceNormals(model.indices.size() / 3);
	std::vector<std::vector<uint32_t>> vertexFaces(model.vertices.size());

	CalculateFaceNormals(model, faceNormals, vertexFaces);
	AverageVertexNormals(model, faceNormals, vertexFaces);
}

void ModelManager::CalculateFaceNormals(const ModelResource& model, std::vector<glm::vec3>& faceNormals, std::vector<std::vector<uint32_t>>& vertexFaces)
{
	for (size_t i = 0; i < model.indices.size(); i += 3)
	{
		uint32_t idx0 = model.indices[i];
		uint32_t idx1 = model.indices[i + 1];
		uint32_t idx2 = model.indices[i + 2];

		glm::vec3 v0 = model.vertices[idx0].pos;
		glm::vec3 v1 = model.vertices[idx1].pos;
		glm::vec3 v2 = model.vertices[idx2].pos;

		glm::vec3 faceNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
		faceNormals[i / 3] = faceNormal;

		vertexFaces[idx0].push_back(i / 3);
		vertexFaces[idx1].push_back(i / 3);
		vertexFaces[idx2].push_back(i / 3);
	}
}

void ModelManager::AverageVertexNormals(ModelResource& model, const std::vector<glm::vec3>& faceNormals, const std::vector<std::vector<uint32_t>>& vertexFaces)
{
	for (size_t i = 0; i < model.vertices.size(); ++i)
	{
		glm::vec3 vertexNormal(0.0f);
		for (uint32_t faceIndex: vertexFaces[i])
		{
			vertexNormal += faceNormals[faceIndex];
		}
		model.vertices[i].normal = glm::normalize(vertexNormal);
	}
}

void ModelManager::CalculateTangentsAndBitangents(ModelResource& model)
{
	for (size_t i = 0; i < model.indices.size(); i += 3)
	{
		Vertex& v0 = model.vertices[model.indices[i]];
		Vertex& v1 = model.vertices[model.indices[i + 1]];
		Vertex& v2 = model.vertices[model.indices[i + 2]];

		CalculateTangentSpace(v0, v1, v2);
	}
}

void ModelManager::CalculateTangentSpace(Vertex& v0, Vertex& v1, Vertex& v2)
{
	glm::vec3 edge1 = v1.pos - v0.pos;
	glm::vec3 edge2 = v2.pos - v0.pos;

	glm::vec2 deltaUV1 = v1.texCoord - v0.texCoord;
	glm::vec2 deltaUV2 = v2.texCoord - v0.texCoord;

	float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	glm::vec3 tangent = CalculateTangent(edge1, edge2, deltaUV1, deltaUV2, f);

	v0.tangent += tangent;
	v1.tangent += tangent;
	v2.tangent += tangent;

	v0.bitangent += glm::cross(v0.normal, tangent);
	v1.bitangent += glm::cross(v1.normal, tangent);
	v2.bitangent += glm::cross(v2.normal, tangent);
}

void ModelManager::CreateBuffersForMesh(VmaAllocator allocator, ModelResource& model)
{
	// Create vertex and index buffers
	SlimeUtil::CreateBuffer("Vertex Buffer", allocator, model.vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, model.vertexBuffer, model.vertexAllocation);
	SlimeUtil::CreateBuffer("Index Buffer", allocator, model.indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, model.indexBuffer, model.indexAllocation);

	// Copy vertex and index data to buffers
	void* data;
	vmaMapMemory(allocator, model.vertexAllocation, &data);
	memcpy(data, model.vertices.data(), model.vertices.size() * sizeof(Vertex));
	vmaUnmapMemory(allocator, model.vertexAllocation);

	vmaMapMemory(allocator, model.indexAllocation, &data);
	memcpy(data, model.indices.data(), model.indices.size() * sizeof(uint32_t));
	vmaUnmapMemory(allocator, model.indexAllocation);
}

ModelResource* ModelManager::LoadModel(const std::string& name, const std::string& pipelineName)
{
	std::string fullPath = ResourcePathManager::GetModelPath(name);

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!LoadObjFile(fullPath, attrib, shapes, materials, warn, err))
	{
		return nullptr;
	}

	ModelResource model;
	ProcessVerticesAndIndices(attrib, shapes, model);

	CenterModel(model.vertices);

	if (attrib.texcoords.empty())
	{
		CalculateTexCoords(model.vertices, model.indices);
	}

	if (model.vertices[0].normal == glm::vec3(0.0f))
	{
		CalculateNormals(model);
	}

	CalculateTangentsAndBitangents(model);

	model.pipelineName = pipelineName;

	m_modelResources[name] = std::move(model);
	spdlog::debug("Model '{}' loaded successfully", name);

	return &m_modelResources[name];
}

TextureResource* ModelManager::LoadTexture(vkb::DispatchTable& disp, VkQueue graphicsQueue, VkCommandPool commandPool, VmaAllocator allocator, DescriptorManager* descriptorManager, const std::string& name)
{
	std::string fullPath = ResourcePathManager::GetTexturePath(name);

	if (m_textures.contains(name))
	{
		spdlog::warn("Texture already exists: {}", name);
		return &m_textures[name];
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
	SlimeUtil::CreateBuffer("Load Texture Staging Buffer", allocator, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer, stagingAllocation);

	// Copy pixel data to staging buffer
	void* data;
	vmaMapMemory(allocator, stagingAllocation, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vmaUnmapMemory(allocator, stagingAllocation);

	stbi_image_free(pixels);

	// Create image
	SlimeUtil::CreateImage(name.c_str(), allocator, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY, texture.image, texture.allocation);

	// Transition image layout and copy buffer to image
	TransitionImageLayout(disp, graphicsQueue, commandPool, texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	CopyBufferToImage(disp, graphicsQueue, commandPool, stagingBuffer, texture.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	TransitionImageLayout(disp, graphicsQueue, commandPool, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Create image view
	texture.imageView = CreateImageView(disp, texture.image, VK_FORMAT_R8G8B8A8_SRGB);

	// Create sampler
	texture.sampler = SlimeUtil::CreateSampler(disp);

	// Cleanup staging buffer
	vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);

	m_textures[name] = std::move(texture);
	spdlog::debug("Texture '{}' loaded successfully", name);
	return &m_textures[name];
}

TextureResource* ModelManager::CopyTexture(const std::string& name, TextureResource* texture)
{
	if (m_textures.contains(name))
	{
		spdlog::warn("Texture already exists: {}", name);
		return &m_textures[name];
	}

	TextureResource newTexture = *texture;
	m_textures[name] = std::move(newTexture);
	return &m_textures[name];
}

const TextureResource* ModelManager::GetTexture(const std::string& name) const
{
	auto it = m_textures.find(name);
	if (it != m_textures.end())
	{
		return &it->second;
	}
	return nullptr;
}

void ModelManager::UnloadAllResources(vkb::DispatchTable& disp, VmaAllocator allocator)
{
	for (const auto& model: m_modelResources)
	{
		vmaDestroyBuffer(allocator, model.second.vertexBuffer, model.second.vertexAllocation);
		vmaDestroyBuffer(allocator, model.second.indexBuffer, model.second.indexAllocation);
	}
	m_modelResources.clear();

	for (const auto& texture: m_textures)
	{
		disp.destroyImageView(texture.second.imageView, nullptr);
		vmaDestroyImage(allocator, texture.second.image, texture.second.allocation);
		disp.destroySampler(texture.second.sampler, nullptr);
	}
	m_textures.clear();

	spdlog::debug("All resources unloaded");
}

std::map<std::string, PipelineConfig>& ModelManager::GetPipelines()
{
	return m_pipelines;
}

void ModelManager::CleanUpAllPipelines(vkb::DispatchTable& disp)
{
	for (auto& [name, pipeline]: m_pipelines)
	{
		disp.destroyPipeline(pipeline.pipeline, nullptr);
		disp.destroyPipelineLayout(pipeline.pipelineLayout, nullptr);
	}
	m_pipelines.clear();
}

VkImageView ModelManager::CreateImageView(vkb::DispatchTable& disp, VkImage image, VkFormat format)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (disp.createImageView(&viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

void ModelManager::BindTexture(vkb::DispatchTable& disp, const std::string& name, uint32_t binding, VkDescriptorSet set)
{
	const TextureResource* texture = GetTexture(name);
	if (!texture)
	{
		throw std::runtime_error("Texture not found: " + name);
	}

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = texture->imageView;
	imageInfo.sampler = VK_NULL_HANDLE;

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = set;
	descriptorWrite.dstBinding = binding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	disp.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
}

void ModelManager::TransitionImageLayout(vkb::DispatchTable& disp, VkQueue graphicsQueue, VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = SlimeUtil::BeginSingleTimeCommands(disp, commandPool);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	else
	{
		spdlog::error("unsupported layout transition!");
	}

	disp.cmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	SlimeUtil::EndSingleTimeCommands(disp, graphicsQueue, commandPool, commandBuffer);
}

void ModelManager::CopyBufferToImage(vkb::DispatchTable& disp, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = SlimeUtil::BeginSingleTimeCommands(disp, commandPool);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	disp.cmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	SlimeUtil::EndSingleTimeCommands(disp, graphicsQueue, commandPool, commandBuffer);
}

void ModelManager::CreateShadowMapPipeline(VulkanContext& vulkanContext, ShaderManager& shaderManager, DescriptorManager& descriptorManager)
{
	const std::string pipelineName = "ShadowMap";

	if (m_pipelines.contains(pipelineName))
	{
		spdlog::error("Shadow map pipeline already exists.");
		return;
	}

	std::vector<std::pair<std::string, VkShaderStageFlagBits>> shaderPaths = {
		{ResourcePathManager::GetShaderPath("shadowmap.vert.spv"),   VK_SHADER_STAGE_VERTEX_BIT},
        {ResourcePathManager::GetShaderPath("shadowmap.frag.spv"), VK_SHADER_STAGE_FRAGMENT_BIT}
	};

	// Load and parse shaders
	std::vector<ShaderModule> shaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	for (const auto& [shaderPath, shaderStage]: shaderPaths)
	{
		auto shaderModule = shaderManager.LoadShader(vulkanContext.GetDispatchTable(), shaderPath, shaderStage);
		shaderModules.push_back(shaderModule);
		shaderStages.push_back({ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = shaderStage, .module = shaderModule.handle, .pName = "main" });
	}
	auto combinedResources = shaderManager.CombineResources(shaderModules);

	PipelineGenerator pipelineGenerator;

	pipelineGenerator.SetName(pipelineName);

	// Set up rendering info for dynamic rendering
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
	VkPipelineRenderingCreateInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingInfo.depthAttachmentFormat = depthFormat;
	renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
	renderingInfo.colorAttachmentCount = 0;
	renderingInfo.pColorAttachmentFormats = nullptr;
	pipelineGenerator.SetRenderingInfo(renderingInfo);

	pipelineGenerator.SetShaderStages(shaderStages);

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(combinedResources.bindingDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = combinedResources.bindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(combinedResources.attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = combinedResources.attributeDescriptions.data();
	pipelineGenerator.SetVertexInputState(vertexInputInfo);

	pipelineGenerator.SetDefaultInputAssembly();
	pipelineGenerator.SetDefaultViewportState();

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.lineWidth = 1.0f;
	pipelineGenerator.SetRasterizationState(rasterizer);

	// Set up multisample state
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;
	pipelineGenerator.SetMultisampleState(multisampling);

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;
	pipelineGenerator.SetDepthStencilState(depthStencil);

	// For shadow mapping, we typically don't use color attachments
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 0;
	colorBlending.pAttachments = nullptr;
	pipelineGenerator.SetColorBlendState(colorBlending);

	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT, VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT, VK_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT, VK_DYNAMIC_STATE_LINE_WIDTH };
	pipelineGenerator.SetDynamicState({ .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() });

	pipelineGenerator.SetPushConstantRanges(combinedResources.pushConstantRanges);

	PipelineConfig config = pipelineGenerator.Build(vulkanContext.GetDispatchTable(), vulkanContext.GetDebugUtils());

	// Store the pipeline
	m_pipelines[pipelineName] = config;

	spdlog::debug("Created the Shadow Map Pipeline");
}

void ModelManager::CreatePipeline(const std::string& pipelineName, VulkanContext& vulkanContext, ShaderManager& shaderManager, DescriptorManager& descriptorManager, const std::vector<std::pair<std::string, VkShaderStageFlagBits>>& shaderPaths, bool depthTestEnabled, VkCullModeFlags cullMode, VkPolygonMode polygonMode)
{
	if (m_pipelines.contains(pipelineName))
	{
		spdlog::error("Pipeline with name '{}' already exists.", pipelineName);
		return;
	}

	// Load and parse shaders
	std::vector<ShaderModule> shaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	for (const auto& [shaderPath, shaderStage]: shaderPaths)
	{
		auto shaderModule = shaderManager.LoadShader(vulkanContext.GetDispatchTable(), shaderPath, shaderStage);
		shaderModules.push_back(shaderModule);
		shaderStages.push_back({ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = shaderStage, .module = shaderModule.handle, .pName = "main" });
	}
	auto combinedResources = shaderManager.CombineResources(shaderModules);

	// Set up descriptor set layout
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = shaderManager.CreateDescriptorSetLayouts(vulkanContext.GetDispatchTable(), combinedResources);

	VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

	PipelineGenerator pipelineGenerator;

	pipelineGenerator.SetName(pipelineName);

	VkPipelineRenderingCreateInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachmentFormats = &colorFormat;
	renderingInfo.depthAttachmentFormat = depthFormat;
	renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
	pipelineGenerator.SetRenderingInfo(renderingInfo);

	pipelineGenerator.SetShaderStages(shaderStages);

	// Set vertex input state only if vertex shader is present
	if (std::find_if(shaderPaths.begin(), shaderPaths.end(), [](const auto& pair) { return pair.second == VK_SHADER_STAGE_VERTEX_BIT; }) != shaderPaths.end())
	{
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(combinedResources.bindingDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = combinedResources.bindingDescriptions.data();
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(combinedResources.attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = combinedResources.attributeDescriptions.data();
		pipelineGenerator.SetVertexInputState(vertexInputInfo);
	}

	pipelineGenerator.SetDefaultInputAssembly();
	pipelineGenerator.SetDefaultViewportState();

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = polygonMode;
	rasterizer.cullMode = cullMode;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.lineWidth = 1.0f;
	pipelineGenerator.SetRasterizationState(rasterizer);

	pipelineGenerator.SetDefaultMultisampleState();

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = depthTestEnabled;
	depthStencil.depthWriteEnable = depthTestEnabled;
	depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; // Assuming reverse-Z
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;
	pipelineGenerator.SetDepthStencilState(depthStencil);

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	pipelineGenerator.SetColorBlendState(colorBlending);

	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT, VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT, VK_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT, VK_DYNAMIC_STATE_LINE_WIDTH };
	pipelineGenerator.SetDynamicState({ .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() });

	pipelineGenerator.SetDescriptorSetLayouts(descriptorSetLayouts);
	pipelineGenerator.SetPushConstantRanges(combinedResources.pushConstantRanges);

	PipelineConfig config = pipelineGenerator.Build(vulkanContext.GetDispatchTable(), vulkanContext.GetDebugUtils());

	// Store the pipeline
	m_pipelines[pipelineName] = config;

	spdlog::debug("Created pipeline: {}", pipelineName);
}

ModelResource* ModelManager::CreateLinePlane(VmaAllocator allocator)
{
	std::string name = "linePlane";
	if (m_modelResources.contains(name))
	{
		return &m_modelResources[name];
	}

	ModelResource model;
	model.pipelineName = "debug_wire";

	// Create vertices
	Vertex vertex;
	vertex.pos = glm::vec3(-1.0f, 0.0f, -1.0f);
	vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
	vertex.texCoord = glm::vec2(0.0f, 0.0f);
	vertex.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
	vertex.bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
	model.vertices.push_back(vertex);

	vertex.pos = glm::vec3(1.0f, 0.0f, -1.0f);
	model.vertices.push_back(vertex);

	vertex.pos = glm::vec3(1.0f, 0.0f, 1.0f);
	model.vertices.push_back(vertex);

	vertex.pos = glm::vec3(-1.0f, 0.0f, 1.0f);
	model.vertices.push_back(vertex);

	model.indices = { 0, 1, 1, 2, 2, 3, 3, 0 };

	m_modelResources[name] = std::move(model);
	spdlog::debug("{} generated.", name);

	return &m_modelResources[name];
}

ModelResource* ModelManager::CreatePlane(VmaAllocator allocator, float size, int divisions)
{
	std::string name = "plane" + std::to_string(size) + "_" + std::to_string(divisions);
	if (m_modelResources.contains(name))
	{
		return &m_modelResources[name];
	}
	ModelResource model;
	model.pipelineName = "pbr";
	// Calculate the step size
	float step = size / divisions;
	// Create vertices
	for (int i = 0; i <= divisions; ++i)
	{
		for (int j = 0; j <= divisions; ++j)
		{
			float x = -size / 2 + i * step;
			float z = -size / 2 + j * step;
			Vertex vertex;
			vertex.pos = glm::vec3(x, 0.0f, z);
			vertex.normal = glm::vec3(0.0f, 1.0f, -1.0f);
			// Calculate UV coordinates to be 0-1 for each quad, but uniform direction
			vertex.texCoord = glm::vec2(static_cast<float>(i % 2), static_cast<float>(1 - (j % 2)));
			vertex.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
			vertex.bitangent = glm::vec3(0.0f, 0.0f, -1.0f);
			model.vertices.push_back(vertex);
		}
	}
	// Create indices for triangles
	for (int i = 0; i < divisions; ++i)
	{
		for (int j = 0; j < divisions; ++j)
		{
			int topLeft = i * (divisions + 1) + j;
			int topRight = topLeft + 1;
			int bottomLeft = (i + 1) * (divisions + 1) + j;
			int bottomRight = bottomLeft + 1;
			// First triangle
			model.indices.push_back(topLeft);
			model.indices.push_back(topRight);
			model.indices.push_back(bottomLeft);
			// Second triangle
			model.indices.push_back(topRight);
			model.indices.push_back(bottomRight);
			model.indices.push_back(bottomLeft);
		}
	}
	m_modelResources[name] = std::move(model);
	spdlog::debug("{} generated.", name);
	return &m_modelResources[name];
}

ModelResource* ModelManager::CreateCube(VmaAllocator allocator, float size)
{
	std::string name = "cube_" + std::to_string(size);
	if (m_modelResources.contains(name))
	{
		return &m_modelResources[name];
	}
	ModelResource model;
	model.pipelineName = "pbr"; // Assume a default pipeline for basic shapes can be changed after
	float halfSize = size / 2.0f;
	// Define the 8 vertices of the cube
	std::vector<glm::vec3> positions = {
		{-halfSize, -halfSize, -halfSize}, // 0: left-bottom-front
		{ halfSize, -halfSize, -halfSize}, // 1: right-bottom-front
		{ halfSize,  halfSize, -halfSize}, // 2: right-top-front
		{-halfSize,  halfSize, -halfSize}, // 3: left-top-front
		{-halfSize, -halfSize,  halfSize}, // 4: left-bottom-back
		{ halfSize, -halfSize,  halfSize}, // 5: right-bottom-back
		{ halfSize,  halfSize,  halfSize}, // 6: right-top-back
		{-halfSize,  halfSize,  halfSize}  // 7: left-top-back
	};
	// Define the 6 face normals
	std::vector<glm::vec3> normals = {
		{ 0.0f,  0.0f, -1.0f}, // Front
		{ 0.0f,  0.0f,  1.0f}, // Back
		{ 1.0f,  0.0f,  0.0f}, // Right
		{-1.0f,  0.0f,  0.0f}, // Left
		{ 0.0f,  1.0f,  0.0f}, // Top
		{ 0.0f, -1.0f,  0.0f}  // Bottom
	};
	// Define the vertices for each face (corrected winding order)
	const int faceVertices[6][4] = {
		{0, 3, 2, 1}, // Front face
		{5, 6, 7, 4}, // Back face
		{1, 2, 6, 5}, // Right face
		{4, 7, 3, 0}, // Left face
		{3, 7, 6, 2}, // Top face
		{4, 0, 1, 5}  // Bottom face
	};
	// Define UVs for each face
	const glm::vec2 faceUVs[4] = {
		{0.0f, 1.0f}, // bottom-left
		{0.0f, 0.0f}, // top-left
		{1.0f, 0.0f}, // top-right
		{1.0f, 1.0f}  // bottom-right
	};
	// Create vertices and indices
	std::vector<uint32_t> newIndices;
	uint32_t vertexCount = 0;
	for (int face = 0; face < 6; ++face)
	{
		for (int i = 0; i < 4; ++i) // 4 vertices per face
		{
			Vertex vertex;
			vertex.pos = positions[faceVertices[face][i]];
			vertex.normal = normals[face];
			vertex.texCoord = faceUVs[i];
			// Tangent space calculation
			glm::vec3 tangent, bitangent;
			if (face == 0 || face == 1) // Front and Back faces
			{
				tangent = glm::vec3(1.0f, 0.0f, 0.0f);
				bitangent = glm::vec3(0.0f, -1.0f, 0.0f);
			}
			else if (face == 2 || face == 3) // Right and Left faces
			{
				tangent = glm::vec3(0.0f, 0.0f, -1.0f);
				bitangent = glm::vec3(0.0f, -1.0f, 0.0f);
			}
			else // Top and Bottom faces
			{
				tangent = glm::vec3(1.0f, 0.0f, 0.0f);
				bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
			}
			vertex.tangent = tangent;
			vertex.bitangent = bitangent;
			model.vertices.push_back(vertex);
			vertexCount++;
		}
		// Add indices for two triangles
		newIndices.push_back(vertexCount - 4);
		newIndices.push_back(vertexCount - 3);
		newIndices.push_back(vertexCount - 2);
		newIndices.push_back(vertexCount - 4);
		newIndices.push_back(vertexCount - 2);
		newIndices.push_back(vertexCount - 1);
	}
	// Assign the new indices to the model
	model.indices = newIndices;
	m_modelResources[name] = std::move(model);
	spdlog::debug("{} generated.", name);
	return &m_modelResources[name];
}

ModelResource* ModelManager::CreateSphere(VmaAllocator allocator, float radius, int segments, int rings)
{
	std::string name = "debug_sphere" + std::to_string(radius) + "_" + std::to_string(segments) + "_" + std::to_string(rings);
	if (m_modelResources.contains(name))
	{
		return &m_modelResources[name];
	}

	ModelResource model;
	model.pipelineName = "pbr";

	for (int ring = 0; ring <= rings; ++ring)
	{
		float theta = ring * glm::pi<float>() / rings;
		float sinTheta = std::sin(theta);
		float cosTheta = std::cos(theta);

		for (int segment = 0; segment <= segments; ++segment)
		{
			float phi = segment * 2 * glm::pi<float>() / segments;
			float sinPhi = std::sin(phi);
			float cosPhi = std::cos(phi);

			float x = cosPhi * sinTheta;
			float y = cosTheta;
			float z = sinPhi * sinTheta;

			Vertex vertex;
			vertex.pos = glm::vec3(x, y, z) * radius;
			vertex.normal = glm::vec3(x, y, z);
			vertex.texCoord = glm::vec2(static_cast<float>(segment) / segments, static_cast<float>(ring) / rings);
			vertex.tangent = glm::normalize(glm::vec3(-z, 0, x));
			vertex.bitangent = glm::cross(vertex.normal, vertex.tangent);

			model.vertices.push_back(vertex);
		}
	}

	for (int ring = 0; ring < rings; ++ring)
	{
		for (int segment = 0; segment < segments; ++segment)
		{
			int current = ring * (segments + 1) + segment;
			int next = current + segments + 1;

			model.indices.push_back(current);
			model.indices.push_back(next);
			model.indices.push_back(current + 1);

			model.indices.push_back(current + 1);
			model.indices.push_back(next);
			model.indices.push_back(next + 1);
		}
	}

	m_modelResources[name] = std::move(model);
	spdlog::debug("{} generated.", name);

	return &m_modelResources[name];
}

ModelResource* ModelManager::CreateCylinder(VmaAllocator allocator, float radius, float height, int segments)
{
	std::string name = "debug_cylinder" + std::to_string(radius) + "_" + std::to_string(height) + "_" + std::to_string(segments);
	if (m_modelResources.contains(name))
	{
		return &m_modelResources[name];
	}

	ModelResource model;
	model.pipelineName = "pbr";

	float halfHeight = height / 2.0f;

	// Create vertices for the sides
	for (int i = 0; i <= segments; ++i)
	{
		float angle = i * 2 * glm::pi<float>() / segments;
		float x = std::cos(angle) * radius;
		float z = std::sin(angle) * radius;

		glm::vec3 normal(x, 0.0f, z);
		normal = glm::normalize(normal);

		// Bottom vertex
		Vertex bottomVertex;
		bottomVertex.pos = glm::vec3(x, -halfHeight, z);
		bottomVertex.normal = normal;
		bottomVertex.texCoord = glm::vec2(static_cast<float>(i) / segments, 0.0f);
		bottomVertex.tangent = glm::vec3(-z, 0.0f, x);
		bottomVertex.bitangent = glm::cross(bottomVertex.normal, bottomVertex.tangent);
		model.vertices.push_back(bottomVertex);

		// Top vertex
		Vertex topVertex = bottomVertex;
		topVertex.pos.y = halfHeight;
		topVertex.texCoord.y = 1.0f;
		model.vertices.push_back(topVertex);
	}

	// Create indices for the sides
	for (int i = 0; i < segments; ++i)
	{
		int current = i * 2;
		int next = (i + 1) * 2;

		model.indices.push_back(current);
		model.indices.push_back(next);
		model.indices.push_back(current + 1);

		model.indices.push_back(current + 1);
		model.indices.push_back(next);
		model.indices.push_back(next + 1);
	}

	// Create vertices and indices for the top and bottom caps
	for (int cap = 0; cap < 2; ++cap)
	{
		int centerIndex = model.vertices.size();
		Vertex centerVertex;
		centerVertex.pos = glm::vec3(0.0f, cap == 0 ? -halfHeight : halfHeight, 0.0f);
		centerVertex.normal = glm::vec3(0.0f, cap == 0 ? -1.0f : 1.0f, 0.0f);
		centerVertex.texCoord = glm::vec2(0.5f, 0.5f);
		centerVertex.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
		centerVertex.bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
		model.vertices.push_back(centerVertex);

		for (int i = 0; i <= segments; ++i)
		{
			float angle = i * 2 * glm::pi<float>() / segments;
			float x = std::cos(angle) * radius;
			float z = std::sin(angle) * radius;

			Vertex vertex = centerVertex;
			vertex.pos = glm::vec3(x, centerVertex.pos.y, z);
			vertex.texCoord = glm::vec2((std::cos(angle) + 1.0f) / 2.0f, (std::sin(angle) + 1.0f) / 2.0f);
			model.vertices.push_back(vertex);

			if (i < segments)
			{
				if (cap == 0)
				{
					model.indices.push_back(centerIndex);
					model.indices.push_back(centerIndex + i + 1);
					model.indices.push_back(centerIndex + i + 2);
				}
				else
				{
					model.indices.push_back(centerIndex);
					model.indices.push_back(centerIndex + i + 2);
					model.indices.push_back(centerIndex + i + 1);
				}
			}
		}
	}

	m_modelResources[name] = std::move(model);
	spdlog::debug("{} generated.", name);

	return &m_modelResources[name];
}

int ModelManager::DrawModel(vkb::DispatchTable& disp, VkCommandBuffer& cmd, const ModelResource& model)
{
	VkDeviceSize offsets[] = { 0 };
	disp.cmdBindVertexBuffers(cmd, 0, 1, &model.vertexBuffer, offsets);
	disp.cmdBindIndexBuffer(cmd, model.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	disp.cmdDrawIndexed(cmd, static_cast<uint32_t>(model.indices.size()), 1, 0, 0, 0);
	return 0;
}
