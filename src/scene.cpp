#include "scene.h"

#include "vulkanShader.h"
#include "vulkanhelper.h"
#include "spdlog/spdlog.h"

#include <filesystem>

std::string getBuildDirectory()
{
	// Get the absolute path of the current source file
	auto srcPath = std::filesystem::absolute(__FILE__);
	return std::filesystem::path(srcPath).parent_path().parent_path().string();
}

int SetupScean(SlimeEngine::Init& init, SlimeEngine::RenderData& data)
{
	std::string buildDir = getBuildDirectory();
	spdlog::info("Build directory: {}", buildDir);

	// CREATING THE SHADDER MODULES
	std::string shadersDir = buildDir + "/shaders";

	SlimeEngine::VertexInputConfig vertexInputConfig   = {};
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding                         = 0;
	bindingDescription.stride                          = sizeof(SlimeEngine::Vertex);
	bindingDescription.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;

	std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
	attributeDescriptions.resize(2);

	attributeDescriptions[0].binding  = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset   = offsetof(SlimeEngine::Vertex, position);

	attributeDescriptions[1].binding  = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset   = offsetof(SlimeEngine::Vertex, normal);

	vertexInputConfig.bindingDescriptions   = bindingDescription;
	vertexInputConfig.attributeDescriptions = attributeDescriptions;

	SlimeEngine::ShaderConfig shaderConfig =
	{
		.vertShaderPath = shadersDir + "/basic.vert.spv",
		.fragShaderPath = shadersDir + "/basic.frag.spv",
		.vertexInputConfig = vertexInputConfig
	};

	// MODEL LOADING
	SlimeEngine::ModelConfig modelConfig =
	{
		.modelPath = buildDir + "/models/Cube/Cube.gltf"
	};
	data.models["cube"] = SlimeEngine::loadModel(modelConfig.modelPath.c_str(), init);

	// CREATING THE GRAPHICS PIPELINE
	if (SlimeEngine::CreateGraphicsPipeline(init, data, "basic", shaderConfig, {}, {}) != 0)
		return -1;

	return 0;
}