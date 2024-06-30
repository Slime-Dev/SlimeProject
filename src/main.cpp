#include "vulkanhelper.h"
#include "vulkanwindow.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks-inl.h"

#define BUILD_DIRECTORY "/home/alexm/Source/SlimeOdyssey/Shaders"

const int MAX_FRAMES_IN_FLIGHT = 2;

int main()
{
	spdlog::set_level(spdlog::level::trace);
	spdlog::stdout_color_mt("console");

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
	if (SlimeEngine::CreateGraphicsPipeline(init, data, BUILD_DIRECTORY) != 0)
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