#include "scene.h"

#include "spdlog/spdlog.h"
#include "ShaderManager.h"
#include "PipelineGenerator.h"
#include "ResourcePathManager.h"

int SetupScean(Engine& engine)
{
	ResourcePathManager resourcePaths;
	spdlog::info("Root directory: {}", resourcePaths.GetRootDirectory());

	// Load and parse shaders
	ShaderManager& shaderManager = engine.GetShaderManager();
	std::string vertShaderPath   = resourcePaths.GetShaderPath("triangle.vert.spv");
	std::string fragShaderPath   = resourcePaths.GetShaderPath("triangle.frag.spv");

	auto vertexShaderModule = shaderManager.LoadShader(vertShaderPath, VK_SHADER_STAGE_VERTEX_BIT);
	auto fragmentShaderModule = shaderManager.LoadShader(fragShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT);
	auto vertexResources = shaderManager.ParseShader(vertexShaderModule);
	auto fragmentResources = shaderManager.ParseShader(fragmentShaderModule);

	// Set up descriptor set layout
	auto descriptorSetLayout = shaderManager.CreateDescriptorSetLayout(vertexResources.descriptorSetLayoutBindings);

	std::unique_ptr<PipelineGenerator> pipelineGenerator = std::make_unique<PipelineGenerator>(engine.GetDevice());
	pipelineGenerator->SetShaderModules(vertexShaderModule, fragmentShaderModule);
	pipelineGenerator->SetVertexInputState(vertexResources.attributeDescriptions, vertexResources.bindingDescription);
	pipelineGenerator->SetDescriptorSetLayouts({descriptorSetLayout});
	pipelineGenerator->SetPushConstantRanges(vertexResources.pushConstantRanges);
	pipelineGenerator->Generate();

	engine.GetPipelines()["basic"] = std::move(pipelineGenerator);

	// MODEL LOADING TODO Implement model loading
	// std::string modelPath = buildDir + "/models/Cube/Cube.gltf";
	// engine.GetModels()["cube"] = SlimeEngine::loadModel(modelPath.c_str(), engine);

	return 0;
}