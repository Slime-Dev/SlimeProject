#pragma once

#include <map>

#include <vk_mem_alloc.h>
#include <string>

#include <tiny_gltf.h>

namespace SlimeEngine
{
struct Init;

struct bufferData
{
	VkBuffer buffer;
	VmaAllocation allocation;
	uint32_t vertexCount;
	uint32_t indexCount;
	uint32_t firstIndex;
	int32_t vertexOffset;
};

struct ModelConfig
{
	std::string modelPath;

	tinygltf::Model model;
	std::map<int, bufferData> buffers;
};

enum class BufferIndex
{
	VERTEX_BUFFER = 0,
	INDEX_BUFFER = 1,
	NUM_BUFFERS
};

void bindMesh(std::map<int, bufferData>& buffers, Init& init, tinygltf::Model& model, tinygltf::Mesh& mesh);
void bindModelNodes(std::map<int, bufferData>& buffers, Init& init, tinygltf::Model& model, tinygltf::Node& node);
std::pair<VkBuffer, std::map<int, bufferData>> bindModel(Init& init, tinygltf::Model& model);

ModelConfig loadModel(const char* filename, Init& init);
void drawModel(VkCommandBuffer commandBuffer, ModelConfig& modelConfig, Init& init);
}