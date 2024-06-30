
#include "vulkan/vulkanhelper.h"
#include "vulkan/window.h"

#define BUILD_DIRECTORY "F:/PersonalProjects/SlimeOdyssey/Shaders"

const int MAX_FRAMES_IN_FLIGHT = 2;

int main() {
	SlimeEngine::Init init;
	SlimeEngine::RenderData data;

	if (SlimeEngine::DeviceInit(init) != 0) return -1;
	if (SlimeEngine::CreateCommandPool(init, data) != 0) return -1;
	if (SlimeEngine::GetQueues(init, data) != 0) return -1;
	if (SlimeEngine::CreateSwapchain(init, data) != 0) return -1;
	if (SlimeEngine::CreateGraphicsPipeline(init, data, BUILD_DIRECTORY) != 0) return -1;
	if (SlimeEngine::RecordCommandBuffers(init, data) != 0) return -1;
	if (SlimeEngine::InitSyncObjects(init, data) != 0) return -1;

	// Main loop
	while (!SlimeEngine::ShouldClose(init.window)) {
		SlimeEngine::PollEvents();
		SlimeEngine::RenderFrame(init, data);
	}

	// Cleanup
	SlimeEngine::Cleanup(init, data);

	return 0;
}
