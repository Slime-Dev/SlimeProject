#include "scene.h"
#include "vulkanDescriptorSet.h"
#include "vulkanhelper.h"
#include "vulkanwindow.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks-inl.h"

#include <string>

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
	if (SlimeEngine::CreateDefaultDescriptorSetLayouts(init, data) != 0)
		return -1;

	if (SetupScean(init, data) != 0)
		return -1;

	if (SlimeEngine::CreateRenderCommandBuffers(init, data) != 0)
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