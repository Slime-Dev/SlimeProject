#pragma once

#include "vulkanModel.h"
#include <vector>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

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
	VmaAllocator allocator;
};

struct RenderData
{
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;

	std::map<const char*, VkPipelineLayout> pipelineLayout;
	std::map<const char*, VkPipeline> graphicsPipeline;
	std::map<const char*, VkDescriptorSetLayout> descriptorSetLayout;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> renderCommandBuffers;

	std::vector<VkSemaphore> availableSemaphores;
	std::vector<VkSemaphore> finishedSemaphore;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imageInFlight;
	size_t currentFrame = 0;

	std::map<const char*, ModelConfig> models; // TODO: This should be a map and I dont really want it in this struct
};

VkCommandBuffer BeginSingleTimeCommands(Init& init, RenderData& data);
void EndSingleTimeCommands(Init& init, RenderData& data, VkCommandBuffer command_buffer);
int DeviceInit(Init& init);
int CreateImages(Init& init, RenderData& data);
int CreateSwapchain(Init& init, RenderData& data);
int GetQueues(Init& init, RenderData& data);
VkShaderModule CreateShaderModule(Init& init, const std::vector<char>& code);
int CreateCommandPool(Init& init, RenderData& data);
int CreateRenderCommandBuffers(Init& init, RenderData& data);
int InitSyncObjects(Init& init, RenderData& data);
int RenderFrame(Init& init, RenderData& data);

int Cleanup(Init& init, RenderData& data);
} // namespace SlimeEngine