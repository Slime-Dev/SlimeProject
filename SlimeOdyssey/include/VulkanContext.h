//
// Created by alexm on 3/07/24.
//

#pragma once

#include <vector>
#include <vk_mem_alloc.h>

#include "VulkanDebugUtils.h"
#include "Renderer.h"
#include "SlimeWindow.h"
#include <imgui.h>

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

	int CreateContext(SlimeWindow* window);
	int RenderFrame(ModelManager& modelManager, DescriptorManager& descriptorManager, SlimeWindow* window, Scene* scene);
	int Cleanup(ShaderManager& shaderManager, ModelManager& modelManager, DescriptorManager& descriptorManager);

	// Getters
	VulkanDebugUtils& GetDebugUtils();
	VkDevice GetDevice() const;
	const vkb::Swapchain& GetSwapchain() const;
	VkQueue GetGraphicsQueue() const;
	VkQueue GetPresentQueue() const;
	VkCommandPool GetCommandPool() const;
	VmaAllocator GetAllocator() const;
	vkb::DispatchTable& GetDispatchTable();

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
	int GenerateShadowMap(VkCommandBuffer& cmd, ModelManager& modelManager, DescriptorManager& descriptorManager, Scene* scene);

	// Rendering methods
	int Draw(VkCommandBuffer& cmd, int imageIndex, ModelManager& modelManager, DescriptorManager& descriptorManager, Scene* scene);

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

	// Shadows
	void CreateShadowMapStagingBuffer();
	void DestroyShadowMapStagingBuffer();

	float GetShadowMapPixelValue(ModelManager& modelManager, int x, int y);
	void RenderShadowMapInspector(ModelManager& modelManager);
	TextureResource m_shadowMap;
	ImTextureID m_shadowMapId;
	float m_shadowMapZoom = 1.0f;
	VkBuffer m_shadowMapStagingBuffer = VK_NULL_HANDLE;
	VmaAllocation m_shadowMapStagingBufferAllocation = VK_NULL_HANDLE;
	VkDeviceSize m_shadowMapStagingBufferSize = 0;

	// Depth image might want this elsewhere?
	VkImage m_depthImage = VK_NULL_HANDLE;
	VkImageView m_depthImageView = VK_NULL_HANDLE;
	VmaAllocation m_depthImageAllocation;

	Renderer m_renderer;

	VulkanDebugUtils m_debugUtils;

	// Safety checks
	bool m_cleanUpFinished = false;
};
