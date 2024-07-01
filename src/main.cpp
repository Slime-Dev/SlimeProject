#include "vulkanhelper.h"
#include "vulkanGraphicsPipeline.h"
#include "vulkanwindow.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks-inl.h"

#include <filesystem>
#include <string>

std::string getBuildDirectory()
{
	// Get the absolute path of the current source file
	auto srcPath = std::filesystem::absolute(__FILE__);
	return std::filesystem::path(srcPath).parent_path().parent_path().string();
}

int main()
{
	spdlog::set_level(spdlog::level::trace);
	spdlog::stdout_color_mt("console");
	std::string buildDir = getBuildDirectory();

	spdlog::info("Build directory: {}", buildDir);

	std::string shadersDir = buildDir + "/Shaders";

	SlimeEngine::ShaderConfig shaderConfig =
	{
		.vertShaderPath = shadersDir + "/triangle.vert.spv",
		.fragShaderPath = shadersDir + "/triangle.frag.spv"
	 };

	SlimeEngine::Init init;
	SlimeEngine::RenderData data;

	if (SlimeEngine::DeviceInit(init) != 0)
		return -1;
	if (SlimeEngine::CreateCommandPool(init, data) != 0)
		return -1;
	if (SlimeEngine::GetQueues(init, data) != 0)
		return -1;
	if (SlimeEngine::CreateSwapchain(init, data) != 0)
		return -1;
	if (SlimeEngine::CreateGraphicsPipeline(init, data, shaderConfig, {}, {}) != 0)
		return -1;
	if (SlimeEngine::RecordCommandBuffers(init, data) != 0)
		return -1;
	if (SlimeEngine::InitSyncObjects(init, data) != 0)
		return -1;

	// Main loop
	while (!SlimeEngine::ShouldClose(init.window))
	{
		SlimeEngine::PollEvents();
		SlimeEngine::RenderFrame(init, data);
	}

	// Cleanup
	SlimeEngine::Cleanup(init, data);

	return 0;
}