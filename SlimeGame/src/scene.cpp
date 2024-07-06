#include "scene.h"

#include "spdlog/spdlog.h"
#include "ShaderManager.h"
#include "PipelineGenerator.h"
#include "ResourcePathManager.h"
#include "ModelManager.h"

int SetupScean(Engine& engine)
{
	ResourcePathManager resourcePaths;
	spdlog::info("Root directory: {}", resourcePaths.GetRootDirectory());

	// Load and parse shaders
	ShaderManager& shaderManager = engine.GetShaderManager();
	std::string vertShaderPath   = resourcePaths.GetShaderPath("basic.vert.spv");
	std::string fragShaderPath   = resourcePaths.GetShaderPath("basic.frag.spv");

	auto vertexShaderModule   = shaderManager.LoadShader(vertShaderPath, VK_SHADER_STAGE_VERTEX_BIT);
	auto fragmentShaderModule = shaderManager.LoadShader(fragShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT);
	auto vertexResources      = shaderManager.ParseShader(vertexShaderModule);
	auto fragmentResources    = shaderManager.ParseShader(fragmentShaderModule);

	// Set up descriptor set layout
	auto descriptorSetLayout = shaderManager.CreateDescriptorSetLayout(vertexResources.descriptorSetLayoutBindings);

	std::unique_ptr<PipelineGenerator> pipelineGenerator = std::make_unique<PipelineGenerator>(engine.GetDevice());
	pipelineGenerator->SetShaderModules(vertexShaderModule, fragmentShaderModule);
	pipelineGenerator->SetVertexInputState(vertexResources.attributeDescriptions, vertexResources.bindingDescriptions);
	pipelineGenerator->SetDescriptorSetLayouts({ descriptorSetLayout });
	pipelineGenerator->SetPushConstantRanges(vertexResources.pushConstantRanges);
	pipelineGenerator->Generate();

	engine.GetPipelines()["basic"] = std::move(pipelineGenerator);

	// MODEL LOADING
	ModelManager& modelManager = engine.GetModelManager();
	modelManager.LoadModel("cube.obj");

	return 0;
}