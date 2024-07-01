#include "vulkanModel.h"

#include "vulkanhelper.h"
#include <spdlog/spdlog.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "vk_mem_alloc.h"
#include <tiny_gltf.h>

namespace SlimeEngine
{
int loadModelInternal(tinygltf::Model& model, const char* filename)
{
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	const bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
	if (!warn.empty())
	{
		spdlog::warn("{}", warn);
	}

	if (!err.empty())
	{
		spdlog::error("{}", err);
		return -1;
	}

	return res ? 0 : -1;
}

void bindMesh(std::map<int, bufferData>& buffers, Init& init, tinygltf::Model& model, tinygltf::Mesh& mesh)
{
	for (size_t i = 0; i < model.bufferViews.size(); ++i)
	{
		const tinygltf::BufferView& bufferView = model.bufferViews[i];
		const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

		VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;

		// Determine the appropriate buffer usage flags based on the BufferView target
		if (bufferView.target == TINYGLTF_TARGET_ARRAY_BUFFER)
		{
			usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		}
		else if (bufferView.target == TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER)
		{
			usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		}

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferView.byteLength;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

		VkBuffer vkBuffer;
		VmaAllocation allocation;
		if (vmaCreateBuffer(init.allocator, &bufferInfo, &allocInfo, &vkBuffer, &allocation, nullptr) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create buffer!");
		}

		void* data;
		vmaMapMemory(init.allocator, allocation, &data);
		memcpy(data, buffer.data.data() + bufferView.byteOffset, (size_t)bufferView.byteLength);
		vmaUnmapMemory(init.allocator, allocation);

		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;
		uint32_t firstIndex = 0;
		int32_t vertexOffset = 0;

		if (bufferView.target == TINYGLTF_TARGET_ARRAY_BUFFER)
		{
			vertexCount = bufferView.byteLength / sizeof(float) / 3;
		}
		else if (bufferView.target == TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER)
		{
			indexCount = bufferView.byteLength / sizeof(uint32_t);
		}

		buffers[i] = { vkBuffer, allocation, vertexCount, indexCount, firstIndex, vertexOffset };
	}
}

void bindModelNodes(std::map<int, bufferData>& buffers, Init& init, tinygltf::Model& model, tinygltf::Node& node)
{
	if ((node.mesh >= 0) && (node.mesh < model.meshes.size()))
	{
		bindMesh(buffers, init, model, model.meshes[node.mesh]);
	}

	for (size_t i = 0; i < node.children.size(); i++)
	{
		assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));
		bindModelNodes(buffers, init, model, model.nodes[node.children[i]]);
	}
}

std::pair<VkBuffer, std::map<int, bufferData>> bindModel(Init& init, tinygltf::Model& model)
{
	std::map<int, bufferData> buffers;
	std::map<int, VmaAllocation> allocations;

	const tinygltf::Scene& scene = model.scenes[model.defaultScene];
	for (size_t i = 0; i < scene.nodes.size(); ++i)
	{
		assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));
		bindModelNodes(buffers, init, model, model.nodes[scene.nodes[i]]);
	}

	// I might need to store the allocations somewhere so I can free them later
	// not sure just yet will see if it's necessary

	return { buffers[0].buffer, buffers };
}

ModelConfig loadModel(const char* filename, Init& init)
{
	ModelConfig modelConfig;
	modelConfig.modelPath = filename;

	if (loadModelInternal(modelConfig.model, filename) != 0)
	{
		spdlog::error("Failed to load model!");
		return modelConfig;
	}

	auto [buffer, buffers] = bindModel(init, modelConfig.model);
	modelConfig.buffers    = buffers;

	return modelConfig;
}

void drawModel(VkCommandBuffer commandBuffer, ModelConfig& modelConfig, Init& init)
{
	for (const auto& bufferEntry : modelConfig.buffers)
	{
		bufferData bufferData = bufferEntry.second;

		VkBuffer buffer = bufferData.buffer;
		VkDeviceSize offsets[] = { 0 };

		// Assuming it's a vertex buffer for simplicity
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffer, offsets);

		vkCmdDrawIndexed(commandBuffer, bufferData.indexCount, 1, bufferData.firstIndex, bufferData.vertexOffset, 0);
	}
}

}