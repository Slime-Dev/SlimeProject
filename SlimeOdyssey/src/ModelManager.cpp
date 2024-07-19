#include "ModelManager.h"

#include <VulkanContext.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <vk_mem_alloc.h>

// Assuming we're using tinyobj for model loading
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// Assuming we're using stb_image for texture loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "VulkanUtil.h"

ModelManager::ModelManager(ResourcePathManager& pathManager)
      : m_pathManager(pathManager)
{
}

ModelManager::~ModelManager()
{
	// Check for any remaining models or resources
	if (!m_models.empty() || !m_modelResources.empty() || !m_textures.empty())
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

bool ModelManager::LoadObjFile(const std::string& fullPath, tinyobj::attrib_t& attrib, std::vector<tinyobj::shape_t>& shapes, std::vector<tinyobj::material_t>& materials, std::string& warn, std::string& err)
{
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
	SlimeUtil::CreateBuffer("Index Buffer",  allocator, model.indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, model.indexBuffer, model.indexAllocation);

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
	std::string fullPath = m_pathManager.GetModelPath(name);

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

	model.pipeLineName = pipelineName;

	m_modelResources[name] = std::move(model);
	spdlog::info("Model '{}' loaded successfully", name);

	return &m_modelResources[name];
}

const TextureResource* ModelManager::LoadTexture(vkb::DispatchTable& disp, VkQueue graphicsQueue, VkCommandPool commandPool, VmaAllocator allocator, DescriptorManager* descriptorManager, const std::string& name)
{
	std::string fullPath = m_pathManager.GetTexturePath(name);

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
	texture.sampler = descriptorManager->CreateSampler();

	// Cleanup staging buffer
	vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);

	m_textures[name] = std::move(texture);
	spdlog::info("Texture '{}' loaded successfully", name);
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

	for (const auto& model : m_models)
	{
		Material* material = model.second->material;
		if (!material->disposed)
		{
			vmaDestroyBuffer(allocator, material->configBuffer, material->configAllocation);
			material->disposed = true;
		}
	}
	m_models.clear();

	spdlog::info("All resources unloaded");
}

std::map<std::string, PipelineContainer>& ModelManager::GetPipelines()
{
	return m_pipelines;
}

void ModelManager::AddModel(const std::string& name, Model* model)
{
	if (m_models.contains(name))
	{
		throw std::runtime_error("Model already exists: " + name);
	}

	m_models[name] = model;
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
	else
	{
		throw std::invalid_argument("unsupported layout transition!");
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

int ModelManager::DrawModel(vkb::DispatchTable& disp, VkCommandBuffer& cmd, const ModelResource& model)
{
	VkDeviceSize offsets[] = { 0 };
	disp.cmdBindVertexBuffers(cmd, 0, 1, &model.vertexBuffer, offsets);
	disp.cmdBindIndexBuffer(cmd, model.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	disp.cmdDrawIndexed(cmd, static_cast<uint32_t>(model.indices.size()), 1, 0, 0, 0);
	return 0;
}

void ModelManager::AddModelMap(const std::unordered_map<std::string, Model*>& models)
{
	for (const auto& [name, model]: models)
	{
		if (m_models.contains(name))
		{
			throw std::runtime_error("Model already exists: " + name);
		}
		m_models[name] = model;
	}
}

// This will create a new model if it doesn't exist
Model* ModelManager::GetModel(const std::string& name)
{
	if (m_models.contains(name))
	{
		return m_models[name];
	}

	Model* model = new Model();
	m_models[name] = model;
	return model;
}

