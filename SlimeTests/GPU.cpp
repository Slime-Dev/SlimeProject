#include "SlimeWindow.h"
#include "VulkanContext.h"
#include "ShaderManager.h"
#include "ModelManager.h"
#include "DescriptorManager.h"
#include <spdlog/spdlog.h>
#include <vector>
#include <stdexcept>
#include <iostream>


int main() {
	SlimeWindow window = SlimeWindow({ .title = "Slime Odyssey", .width = 1920, .height = 1080, .resizable = true, .decorated = true, .fullscreen = false });

	if (window.GetHeight() != 1080)
	{
		spdlog::error("Failed to set height correctly");
		return 1;
	}
	else if (window.GetWidth() != 1920)
	{
		spdlog::error("Failed to set width correctly");
		return 1;
	}
	else
	{
		spdlog::info("Created window successfully");
	}

	VulkanContext context;
	if (context.CreateContext(&window) != 0)
	{
		spdlog::error("Failed to create context");
		return 1;
	}
	else
	{
		spdlog::info("Created context successfully");
	}

	ShaderManager shaderManager;
	ModelManager modelManager;

	try {
		shaderManager = ShaderManager();
		modelManager = ModelManager();
	}
	catch (const std::exception e) {
		spdlog::error("Failed to initailize managers with error {}", e.what());
		return 1;
	}

	DescriptorManager descriptorManager = DescriptorManager(context.GetDispatchTable());

	spdlog::info("Created managers for cleanup");

	if (context.Cleanup(shaderManager, modelManager, descriptorManager) != 0)
	{
		spdlog::error("Failed to clean up context");
		return 1;
	}
	else
	{
		spdlog::info("Successfully cleaned up context");
	}
}
