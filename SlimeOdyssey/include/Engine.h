//
// Created by alexm on 3/07/24.
//

#pragma once

#include <map>
#include <vector>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>

#include "Camera.h"
#include "DescriptorManager.h"
#include "InputManager.h"
#include "Renderer.h"
#include "SlimeWindow.h"
#include "VulkanDebugUtils.h"

struct TempMaterialTextures;
struct PipelineContainer;
class ShaderManager;
class ModelManager;

struct GLFWwindow;

class Engine
{
public:
	explicit Engine();
	~Engine();

	int CreateEngine(SlimeWindow* window);
	int RenderFrame(ModelManager& modelManager, DescriptorManager& descriptorManager, SlimeWindow* window, Scene& scene);
	int Cleanup(ShaderManager& shaderManager, ModelManager& modelManager, DescriptorManager& descriptorManager);

	// Getters
	VulkanDebugUtils& GetDebugUtils();
	VkDevice GetDevice() const;
	const vkb::Swapchain& GetSwapchain() const;
	VkQueue GetGraphicsQueue() const;
	VkQueue GetPresentQueue() const;
	VkCommandPool GetCommandPool() const;
	VmaAllocator GetAllocator() const;
	const vkb::DispatchTable& GetDispatchTable();

	// Helper methods
	int CreateSwapchain(SlimeWindow* window); // Needs to be public for window resize callback

private:
	// Initialization methods
	int DeviceInit(SlimeWindow* window);
	int GetQueues();
	int CreateCommandPool();
	int CreateRenderCommandBuffers();
	int InitSyncObjects();

	// Rendering methods
	int Draw(VkCommandBuffer& cmd, int imageIndex, ModelManager& modelManager, DescriptorManager& descriptorManager, Scene& scene);

	// Structs
	struct RenderData
	{
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue presentQueue = VK_NULL_HANDLE;
		std::vector<VkImage> swapchainImages;
		std::vector<VkImageView> swapchainImageViews;
		VmaAllocation depthImageAllocation;
		VkImage depthImage = VK_NULL_HANDLE;
		VkImageView depthImageView = VK_NULL_HANDLE;
		VkCommandPool commandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> renderCommandBuffers;
		std::vector<VkSemaphore> availableSemaphores;
		std::vector<VkSemaphore> finishedSemaphore;
		std::vector<VkFence> inFlightFences;
		std::vector<VkFence> imageInFlight;
		size_t currentFrame = 0;
	};

	// Vulkan core
	vkb::Instance m_instance;
	vkb::InstanceDispatchTable m_instDisp;
	VkSurfaceKHR m_surface{};
	vkb::Device m_device;
	vkb::DispatchTable m_disp;
	vkb::Swapchain m_swapchain;
	VmaAllocator m_allocator{};

	// Render data
	RenderData data;
	Renderer m_renderer;

	// Resource managers
	VulkanDebugUtils m_debugUtils;

	// Safety checks
	bool m_cleanUpFinished = false;
};
