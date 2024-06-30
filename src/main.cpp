#include "vulkanhelper.h"
#include "vulkanwindow.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks-inl.h"

#include <filesystem>
#include <string>
namespace fs = std::filesystem; // For convenience

// Function to get the build directory relative to the source file
std::string getBuildDirectory() {
	// Get the absolute path of the current source file
	auto srcPath = fs::absolute(__FILE__);

	return fs::path(srcPath).parent_path().parent_path().string();
}
int main()
{
	spdlog::set_level(spdlog::level::trace);
	spdlog::stdout_color_mt("console");
	std::string buildDir = getBuildDirectory();

	spdlog::info("Build directory: {}", buildDir);

	const std::string shadersDir = buildDir + "/Shaders";

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
	if (SlimeEngine::CreateGraphicsPipeline(init, data, shadersDir.c_str()) != 0)
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