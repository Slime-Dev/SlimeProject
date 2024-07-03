#include "scene.h"

#include "spdlog/spdlog.h"
#include "ShaderManager.h"
#include "PipelineGenerator.h"

#include <filesystem>

std::string getBuildDirectory()
{
	// Get the absolute path of the current source file
	auto srcPath = std::filesystem::absolute(__FILE__);
	return std::filesystem::path(srcPath).parent_path().parent_path().string();
}

int SetupScean(Engine& engine)
{
	std::string buildDir = getBuildDirectory();
	spdlog::info("Build directory: {}", buildDir);

	// CREATING THE SHADDER MODULES
	ShaderManager& shaderManager = engine.GetShaderManager();
	std::string shadersDir = buildDir + "/shaders";
	std::string vertShaderPath = shadersDir + "/basic.vert.spv";
	std::string fragShaderPath = shadersDir + "/basic.frag.spv";
	auto [vertShaderModule, vertShaderCode] = shaderManager.loadShader(vertShaderPath);
	auto [fragShaderModule, fragShaderCode] = shaderManager.loadShader(fragShaderPath);

	std::unique_ptr<PipelineGenerator> pipelineGenerator = std::make_unique<PipelineGenerator>(engine.GetDevice(), vertShaderCode, fragShaderCode);
	pipelineGenerator->setShaderModules(vertShaderModule, fragShaderModule);
	pipelineGenerator->generate();

	engine.GetPipelines()["basic"] = std::move(pipelineGenerator);

	// MODEL LOADING TODO Implement model loading
	// std::string modelPath = buildDir + "/models/Cube/Cube.gltf";
	// engine.GetModels()["cube"] = SlimeEngine::loadModel(modelPath.c_str(), engine);

	return 0;
}