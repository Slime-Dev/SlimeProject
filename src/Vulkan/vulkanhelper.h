#pragma once

#include <vector>

#include <VkBootstrap.h>

struct GLFWwindow;

namespace SlimeEngine
{
struct Init
{
	GLFWwindow* window;
	vkb::Instance instance;
	vkb::InstanceDispatchTable instDisp;
	VkSurfaceKHR surface;
	vkb::Device device;
	vkb::DispatchTable disp;
	vkb::Swapchain swapchain;
};

struct RenderData
{
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;

	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> availableSemaphores;
	std::vector<VkSemaphore> finishedSemaphore;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imageInFlight;
	size_t currentFrame = 0;
};

VkCommandBuffer BeginSingleTimeCommands(Init& init, RenderData& data);
void EndSingleTimeCommands(Init& init, RenderData& data, VkCommandBuffer command_buffer);
int DeviceInit(Init& init);
int CreateImages(Init& init, RenderData& data);
int CreateSwapchain(Init& init, RenderData& data);
int GetQueues(Init& init, RenderData& data);
VkShaderModule CreateShaderModule(Init& init, const std::vector<char>& code);
int CreateGraphicsPipeline(Init& init, RenderData& data, const char* shaderDir);
int CreateCommandPool(Init& init, RenderData& data);
int RecordCommandBuffers(Init& init, RenderData& data);
int InitSyncObjects(Init& init, RenderData& data);
int RenderFrame(Init& init, RenderData& data);

int Cleanup(Init& init, RenderData& data);
} // namespace SlimeEngine