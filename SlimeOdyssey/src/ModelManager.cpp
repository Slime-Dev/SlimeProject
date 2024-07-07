#include "ModelManager.h"
#include <Engine.h>
#include <fstream>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <vk_mem_alloc.h>

// Assuming we're using tinyobj for model loading
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// Assuming we're using stb_image for texture loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

ModelManager::ModelManager(Engine* engine, VkDevice device, VmaAllocator allocator, ResourcePathManager& pathManager)
	: m_engine(engine), m_device(device), m_allocator(allocator), m_pathManager(pathManager)
{
}

ModelManager::~ModelManager()
{
	UnloadAllResources();
}

ModelManager::ModelResource* ModelManager::LoadModel(const std::string& name, const std::string& pipelineName)
{
    std::string fullPath = m_pathManager.GetModelPath(name);

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fullPath.c_str()))
    {
        spdlog::error("Failed to load model '{}': {}", name, err);
		return nullptr;
    }

    ModelResource model;
    std::unordered_map<Vertex, uint32_t> uniqueVertices;

    // Process vertices and indices
    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            if (!attrib.texcoords.empty())
            {
                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }

            if (!attrib.normals.empty())
            {
                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            }

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(model.vertices.size());
                model.vertices.push_back(vertex);
            }

            model.indices.push_back(uniqueVertices[vertex]);
        }
    }

	// Check if we need to calculate normals
	if (model.vertices[0].normal == glm::vec3(0.0f))
	{
		// Calculate normals
		std::vector<glm::vec3> faceNormals(model.indices.size() / 3);
		std::vector<std::vector<uint32_t>> vertexFaces(model.vertices.size());

		// Calculate face normals and store vertex-face associations
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

		// Calculate vertex normals by averaging face normals
		for (size_t i = 0; i < model.vertices.size(); ++i)
		{
			glm::vec3 vertexNormal(0.0f);
			for (uint32_t faceIndex : vertexFaces[i])
			{
				vertexNormal += faceNormals[faceIndex];
			}
			model.vertices[i].normal = glm::normalize(vertexNormal);
		}
	}

	// Calculate tangents
	for (size_t i = 0; i < model.indices.size(); i += 3)
	{
		Vertex& v0 = model.vertices[model.indices[i]];
		Vertex& v1 = model.vertices[model.indices[i + 1]];
		Vertex& v2 = model.vertices[model.indices[i + 2]];

		glm::vec3 edge1 = v1.pos - v0.pos;
		glm::vec3 edge2 = v2.pos - v0.pos;

		glm::vec2 deltaUV1 = v1.texCoord - v0.texCoord;
		glm::vec2 deltaUV2 = v2.texCoord - v0.texCoord;

		float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

		glm::vec3 tangent;
		tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
		tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
		tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

		v0.tangent += tangent;
		v1.tangent += tangent;
		v2.tangent += tangent;
	}

    // Create vertex and index buffers
    m_engine->CreateBuffer(model.vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, model.vertexBuffer, model.vertexAllocation);
    m_engine->CreateBuffer(model.indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, model.indexBuffer, model.indexAllocation);

    // Copy vertex and index data to buffers
    void* data;
    vmaMapMemory(m_allocator, model.vertexAllocation, &data);
    memcpy(data, model.vertices.data(), model.vertices.size() * sizeof(Vertex));
    vmaUnmapMemory(m_allocator, model.vertexAllocation);

    vmaMapMemory(m_allocator, model.indexAllocation, &data);
    memcpy(data, model.indices.data(), model.indices.size() * sizeof(uint32_t));
    vmaUnmapMemory(m_allocator, model.indexAllocation);

	model.pipeLineName = pipelineName.c_str();

    m_models[name] = std::move(model);
    spdlog::info("Model '{}' loaded successfully", name);

	return &m_models[name];
}

bool ModelManager::LoadTexture(const std::string& name)
{
	std::string fullPath = m_pathManager.GetTexturePath(name);

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(fullPath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		spdlog::error("Failed to load texture '{}': {}", name, stbi_failure_reason());
		return false;
	}

	VkDeviceSize imageSize = texWidth * texHeight * 4;

	TextureResource texture;
	texture.width  = texWidth;
	texture.height = texHeight;

	// Create staging buffer
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	m_engine->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
		stagingBuffer, stagingAllocation);

	// Copy pixel data to staging buffer
	void* data;
	vmaMapMemory(m_allocator, stagingAllocation, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vmaUnmapMemory(m_allocator, stagingAllocation);

	stbi_image_free(pixels);

	// Create image
	CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY, texture.image, texture.allocation);

	// Transition image layout and copy buffer to image
	TransitionImageLayout(texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	CopyBufferToImage(stagingBuffer, texture.image, static_cast<uint32_t>(texWidth),
		static_cast<uint32_t>(texHeight));
	TransitionImageLayout(texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Create image view
	texture.imageView = CreateImageView(texture.image, VK_FORMAT_R8G8B8A8_SRGB);

	// Cleanup staging buffer
	vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);

	m_textures[name] = std::move(texture);
	spdlog::info("Texture '{}' loaded successfully", name);
	return true;
}

const ModelManager::ModelResource* ModelManager::GetModel(const std::string& name) const
{
	auto it = m_models.find(name);
	if (it != m_models.end())
	{
		return &it->second;
	}
	return nullptr;
}

const ModelManager::TextureResource* ModelManager::GetTexture(const std::string& name) const
{
	auto it = m_textures.find(name);
	if (it != m_textures.end())
	{
		return &it->second;
	}
	return nullptr;
}

void ModelManager::UnloadResource(const std::string& name)
{
	// Unload model
	auto modelIt = m_models.find(name);
	if (modelIt != m_models.end())
	{
		vmaDestroyBuffer(m_allocator, modelIt->second.vertexBuffer, modelIt->second.vertexAllocation);
		vmaDestroyBuffer(m_allocator, modelIt->second.indexBuffer, modelIt->second.indexAllocation);
		m_models.erase(modelIt);
		spdlog::info("Model '{}' unloaded", name);
	}

	// Unload texture
	auto textureIt = m_textures.find(name);
	if (textureIt != m_textures.end())
	{
		vkDestroyImageView(m_device, textureIt->second.imageView, nullptr);
		vmaDestroyImage(m_allocator, textureIt->second.image, textureIt->second.allocation);
		m_textures.erase(textureIt);
		spdlog::info("Texture '{}' unloaded", name);
	}
}

void ModelManager::UnloadAllResources()
{
	for (const auto& model : m_models)
	{
		vmaDestroyBuffer(m_allocator, model.second.vertexBuffer, model.second.vertexAllocation);
		vmaDestroyBuffer(m_allocator, model.second.indexBuffer, model.second.indexAllocation);
	}
	m_models.clear();

	for (const auto& texture : m_textures)
	{
		vkDestroyImageView(m_device, texture.second.imageView, nullptr);
		vmaDestroyImage(m_allocator, texture.second.image, texture.second.allocation);
	}
	m_textures.clear();

	spdlog::info("All resources unloaded");
}

void ModelManager::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
	VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VkImage& image,
	VmaAllocation& allocation)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType     = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width  = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth  = 1;
	imageInfo.mipLevels     = 1;
	imageInfo.arrayLayers   = 1;
	imageInfo.format        = format;
	imageInfo.tiling        = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage         = usage;
	imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = memoryUsage;

	if (vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image!");
	}
}

VkImageView ModelManager::CreateImageView(VkImage image, VkFormat format)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image                           = image;
	viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format                          = format;
	viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel   = 0;
	viewInfo.subresourceRange.levelCount     = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount     = 1;

	VkImageView imageView;
	if (vkCreateImageView(m_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

void ModelManager::BindModel(const std::string& name, VkCommandBuffer commandBuffer)
{
	const ModelResource* model = GetModel(name);
	if (!model)
	{
		throw std::runtime_error("Model not found: " + name);
	}

	VkBuffer vertexBuffers[] = { model->vertexBuffer };
	VkDeviceSize offsets[]   = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, model->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

void ModelManager::BindTexture(const std::string& name, VkCommandBuffer commandBuffer, VkPipelineLayout layout,
	uint32_t binding, VkDescriptorSet set)
{
	const TextureResource* texture = GetTexture(name);
	if (!texture)
	{
		throw std::runtime_error("Texture not found: " + name);
	}

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView   = texture->imageView;
	imageInfo.sampler     = VK_NULL_HANDLE;

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet          = set;
	descriptorWrite.dstBinding      = binding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo      = &imageInfo;

	vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
}

void ModelManager::TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = m_engine->BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout                       = oldLayout;
	barrier.newLayout                       = newLayout;
	barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.image                           = image;
	barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel   = 0;
	barrier.subresourceRange.levelCount     = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

		sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	m_engine->EndSingleTimeCommands(commandBuffer);
}

void ModelManager::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = m_engine->BeginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset                    = 0;
	region.bufferRowLength                 = 0;
	region.bufferImageHeight               = 0;
	region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel       = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount     = 1;
	region.imageOffset                     = { 0, 0, 0 };
	region.imageExtent                     = { width, height, 1 };

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	m_engine->EndSingleTimeCommands(commandBuffer);
}

int ModelManager::DrawModel(VkCommandBuffer& cmd, const std::string& name)
{
	const ModelResource* model = GetModel(name);
	if (!model)
	{
		spdlog::error("Model not found: {}", name);
		return -1;
	}

	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmd, 0, 1, &model->vertexBuffer, offsets);
	vkCmdBindIndexBuffer(cmd, model->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(cmd, static_cast<uint32_t>(model->indices.size()), 1, 0, 0, 0);
	return 0;
}

int ModelManager::DrawModel(VkCommandBuffer& cmd, const ModelResource& model)
{
	m_engine->GetDebugUtils().BeginDebugMarker(cmd, "Draw Mesh", debugUtil_DrawModelColour);
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmd, 0, 1, &model.vertexBuffer, offsets);
	vkCmdBindIndexBuffer(cmd, model.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(cmd, static_cast<uint32_t>(model.indices.size()), 1, 0, 0, 0);
	m_engine->GetDebugUtils().EndDebugMarker(cmd);
	return 0;
}