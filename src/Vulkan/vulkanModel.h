#pragma once

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include <map>
#include <memory>

#include <vk_mem_alloc.h>
#include <string>
#include <vector>

namespace SlimeEngine
{
struct Init;

struct Vertex
{
	glm::vec3 position;
	glm::vec2 uv; // Packing is most likely broken
	glm::vec3 normal;
	glm::vec4 color;
};

struct bufferData
{
	VkBuffer vertexBuffer;
	VkBuffer indexBuffer;
	VmaAllocation allocation;
	uint32_t vertexCount;
	uint32_t indexCount;
	uint32_t firstIndex;
	int32_t vertexOffset;

	VkDeviceSize offset;
};
struct MeshAsset {
	std::string name;
	bufferData meshBuffers;
};

struct ModelConfig
{
	std::string modelPath;
	std::vector<std::shared_ptr<MeshAsset>> meshAssets; // <vertexBuffer, indexBuffer>
};

struct GeoSurface {
	uint32_t startIndex;
	uint32_t count;
};


ModelConfig loadModel(const char* filename, Init& init);
void drawModel(VkCommandBuffer commandBuffer, ModelConfig& modelConfig, Init& init);
void cleanupModel(Init& init, ModelConfig& modelConfig);
}