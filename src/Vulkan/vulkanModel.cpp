#include "vulkanModel.h"

#include "vulkanhelper.h"
#include <spdlog/spdlog.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <fastgltf/core.hpp>
#include <fastgltf/util.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/base64.hpp>

namespace SlimeEngine
{
VkFilter extract_filter(fastgltf::Filter filter)
{
	switch (filter)
	{
	// nearest samplers
	case fastgltf::Filter::Nearest:
	case fastgltf::Filter::NearestMipMapNearest:
	case fastgltf::Filter::NearestMipMapLinear:
		return VK_FILTER_NEAREST;

	// linear samplers
	case fastgltf::Filter::Linear:
	case fastgltf::Filter::LinearMipMapNearest:
	case fastgltf::Filter::LinearMipMapLinear:
	default:
		return VK_FILTER_LINEAR;
	}
}

VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter)
{
	switch (filter)
	{
	case fastgltf::Filter::NearestMipMapNearest:
	case fastgltf::Filter::LinearMipMapNearest:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;

	case fastgltf::Filter::NearestMipMapLinear:
	case fastgltf::Filter::LinearMipMapLinear:
	default:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
}

int createBuffer(VmaAllocator& allocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer& buffer, VmaAllocation& allocation)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size               = size;
	bufferInfo.usage              = usage;
	bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage                   = memoryUsage;

	if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS)
	{
		spdlog::error("Failed to create buffer!");
		return -1;
	}

	return 0;
}

bufferData uploadMesh(Init& init, const std::vector<uint32_t>& vector, const std::vector<Vertex>& vertices)
{
	// create index buffer
	VkBuffer indexBuffer;
	VmaAllocation indexBufferAllocation;
	VkDeviceSize indexBufferSize = sizeof(vector[0]) * vector.size();
	if (createBuffer(init.allocator, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, indexBuffer, indexBufferAllocation) != 0)
	{
		spdlog::error("Failed to create index buffer!");
	}

	// create vertex buffer
	VkBuffer vertexBuffer;
	VmaAllocation vertexBufferAllocation;
	VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
	if (createBuffer(init.allocator, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, vertexBuffer, vertexBufferAllocation) != 0)
	{
		spdlog::error("Failed to create vertex buffer!");
	}

	return bufferData{ vertexBuffer, indexBuffer, vertexBufferAllocation, static_cast<uint32_t>(vertices.size()), static_cast<uint32_t>(vector.size()), 0, 0, 0 };
}

int loadModelInternal(Init& init, ModelConfig& modelConfig)
{
	fastgltf::Parser parser;
	fastgltf::GltfDataBuffer dataBuffer;

	auto modelPath = modelConfig.modelPath;
	auto path      = std::filesystem::path(modelPath);

	constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

	fastgltf::GltfDataBuffer data;
	data.loadFromFile(modelPath);

	fastgltf::Asset asset;

	auto type = fastgltf::determineGltfFileType(&data);
	if (type == fastgltf::GltfType::glTF)
	{
		auto load = parser.loadGltf(&data, path.parent_path(), gltfOptions);
		if (load)
		{
			asset = std::move(load.get());
		}
		else
		{
			spdlog::error("Failed to load glTF: {}", fastgltf::to_underlying(load.error()));
			return -1;
		}
	}
	else if (type == fastgltf::GltfType::GLB)
	{
		auto load = parser.loadGltfBinary(&data, path.parent_path(), gltfOptions);
		if (load)
		{
			asset = std::move(load.get());
		}
		else
		{
			spdlog::error("Failed to load glTF: {}", fastgltf::to_underlying(load.error()));
			return -1;
		}
	}
	else
	{
		spdlog::error("Failed to determine glTF container");
		return -1;
	}

	// MATERIALS // -----------------------------------------------------------
	// // load samplers
	// std::vector<VkSampler> samplers;
	// for (fastgltf::Sampler& sampler : asset.samplers)
	// {
	//
	// 	VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr };
	// 	sampl.maxLod              = VK_LOD_CLAMP_NONE;
	// 	sampl.minLod              = 0;
	//
	// 	sampl.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
	// 	sampl.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));
	//
	// 	sampl.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));
	//
	// 	VkSampler newSampler;
	// 	vkCreateSampler(init.device, &sampl, nullptr, &newSampler);
	//
	// 	samplers.push_back(newSampler);
	// }
	//
	// todo more material stuff

	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;

	for (fastgltf::Mesh& mesh : asset.meshes)
	{
		std::shared_ptr<MeshAsset> newmesh = std::make_shared<MeshAsset>();
		newmesh->name                      = mesh.name;

		// clear the mesh arrays each mesh, we dont want to merge them by error
		indices.clear();
		vertices.clear();

		for (auto&& p : mesh.primitives)
		{
			size_t initial_vtx = vertices.size();

			// load indexes
			{
				fastgltf::Accessor& indexaccessor = asset.accessors[p.indicesAccessor.value()];
				indices.reserve(indices.size() + indexaccessor.count);

				fastgltf::iterateAccessor<std::uint32_t>(asset, indexaccessor,
					[&](std::uint32_t idx) {
						indices.push_back(idx + initial_vtx);
					});
			}

			// load vertex positions
			{
				fastgltf::Accessor& posAccessor = asset.accessors[p.findAttribute("POSITION")->second];
				vertices.resize(vertices.size() + posAccessor.count);

				fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, posAccessor,
					[&](glm::vec3 v, size_t index) {
						Vertex newvtx;
						newvtx.position               = v;
						newvtx.normal                 = { 1, 0, 0 };
						newvtx.color                  = glm::vec4{ 1.f };
						newvtx.uv                     = { 0, 0 };
						vertices[initial_vtx + index] = newvtx;
					});
			}

			// load vertex normals
			auto normals = p.findAttribute("NORMAL");
			if (normals != p.attributes.end())
			{

				fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, asset.accessors[(*normals).second],
					[&](glm::vec3 v, size_t index) {
						vertices[initial_vtx + index].normal = v;
					});
			}

			// load UVs
			auto uv = p.findAttribute("TEXCOORD_0");
			if (uv != p.attributes.end())
			{

				fastgltf::iterateAccessorWithIndex<glm::vec2>(asset, asset.accessors[(*uv).second],
					[&](glm::vec2 v, size_t index) {
						vertices[initial_vtx + index].uv.x = v.x;
						vertices[initial_vtx + index].uv.y = v.y;
					});
			}

			// load vertex colors
			auto colors = p.findAttribute("COLOR_0");
			if (colors != p.attributes.end())
			{

				fastgltf::iterateAccessorWithIndex<glm::vec4>(asset, asset.accessors[(*colors).second],
					[&](glm::vec4 v, size_t index) {
						vertices[initial_vtx + index].color = v;
					});
			}
		}

		newmesh->meshBuffers = uploadMesh(init, indices, vertices);
	}

	return 0;
}

ModelConfig loadModel(const char* filename, Init& init)
{
	ModelConfig modelConfig;
	modelConfig.modelPath = filename;

	if (loadModelInternal(init, modelConfig) != 0)
	{
		spdlog::error("Failed to load model!");
	}
	return modelConfig;
}

void drawModel(VkCommandBuffer commandBuffer, ModelConfig& modelConfig, Init& init)
{
	for (const auto& meshEntry : modelConfig.meshAssets)
	{
		auto& vertexBuffer = meshEntry->meshBuffers.vertexBuffer;
		auto& indexBuffer  = meshEntry->meshBuffers.indexBuffer;

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &meshEntry->meshBuffers.offset);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, meshEntry->meshBuffers.offset, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffer, meshEntry->meshBuffers.indexCount, 1, meshEntry->meshBuffers.firstIndex, meshEntry->meshBuffers.vertexOffset, 0);
	}
}

void cleanupModel(Init& init, ModelConfig& modelConfig)
{
	for (const auto& meshEntry : modelConfig.meshAssets)
	{
		vmaDestroyBuffer(init.allocator, meshEntry->meshBuffers.vertexBuffer, meshEntry->meshBuffers.allocation);
		vmaDestroyBuffer(init.allocator, meshEntry->meshBuffers.indexBuffer, meshEntry->meshBuffers.allocation);
	}

	modelConfig.meshAssets.clear();
}

}