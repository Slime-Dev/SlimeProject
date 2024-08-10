//
// Created by alexm on 3/07/24.
//

#pragma once

#include <imgui.h>
#include <vector>
#include <vk_mem_alloc.h>

#include "Renderer.h"
#include "SlimeWindow.h"
#include "VulkanDebugUtils.h"

struct MaterialManager;
struct TempMaterialTextures;
struct PipelineContainer;
class ShaderManager;
class ModelManager;

struct GLFWwindow;

class VulkanContext
{
public:
	VulkanContext() = default;
	~VulkanContext();

	int CreateContext(SlimeWindow* window, ModelManager* modelManager);
	int RenderFrame(ModelManager& modelManager, SlimeWindow* window, Scene* scene);
	int Cleanup(ModelManager& modelManager);

	ShaderManager* GetShaderManager();
	DescriptorManager* GetDescriptorManager();

	// Getters
	VulkanDebugUtils& GetDebugUtils();
	VkDevice GetDevice() const;
	const vkb::Swapchain& GetSwapchain() const;
	VkQueue GetGraphicsQueue() const;
	VkQueue GetPresentQueue() const;
	VkCommandPool GetCommandPool() const;
	VmaAllocator GetAllocator() const;
	vkb::DispatchTable& GetDispatchTable();
	MaterialManager* GetMaterialManager();

	// Helper methods
	int CreateSwapchain(SlimeWindow* window); // Needs to be public for window resize callback

private:
	// Initialization methods
	int DeviceInit(SlimeWindow* window);
	int GetQueues();
	int CreateCommandPool();
	int CreateRenderCommandBuffers();
	int InitSyncObjects();
	int InitImGui(SlimeWindow* window);

	// Vulkan core
	vkb::Instance m_instance;
	vkb::InstanceDispatchTable m_instDisp;
	vkb::DispatchTable m_disp;
	VkSurfaceKHR m_surface{};
	vkb::Device m_device;
	vkb::Swapchain m_swapchain;
	VmaAllocator m_allocator{};
	VkCommandPool m_commandPool = VK_NULL_HANDLE;
	VkDescriptorPool m_imguiDescriptorPool = VK_NULL_HANDLE;

	// Render data
	VkQueue m_graphicsQueue = VK_NULL_HANDLE;
	VkQueue m_presentQueue = VK_NULL_HANDLE;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	std::vector<VkCommandBuffer> m_renderCommandBuffers;
	std::vector<VkSemaphore> m_availableSemaphores;
	std::vector<VkSemaphore> m_finishedSemaphore;
	std::vector<VkFence> m_inFlightFences;
	std::vector<VkFence> m_imageInFlight;
	size_t m_currentFrame = 0;

	Renderer m_renderer;

	VulkanDebugUtils m_debugUtils;

	ShaderManager* m_shaderManager = nullptr;
	DescriptorManager* m_descriptorManager = nullptr;
	MaterialManager* m_materialManager = nullptr;

	// Safety checks
	bool m_cleanUpFinished = false;
};
