#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include <vulkan/vulkan.h>

#include "Renderer.h"
#include "RenderPassManager.h"

// Forward declarations
class VulkanDebugUtils;
class ModelManager;
class DescriptorManager;
class Scene;
class SlimeWindow;
class MaterialManager;

class Renderer
{
public:
	Renderer(vkb::Device& device);
	~Renderer();

	void SetUp(vkb::DispatchTable* disp, VmaAllocator allocator, VkSurfaceKHR* surface, VulkanDebugUtils* debugUtils, SlimeWindow* window, ShaderManager* shaderManager, MaterialManager* materialManager, ModelManager* modelManager, DescriptorManager* descriptorManager, VkCommandPool commandPool);

	int RenderFrame(ModelManager& modelManager, SlimeWindow* window, Scene* scene);

	void CreateDepthImage();
	int CreateSwapchain(SlimeWindow* window);

	// Getters
	VkQueue GetGraphicsQueue() const;
	vkb::Swapchain& GetSwapchain();

private:
	int Draw(VkCommandBuffer& cmd, VkCommandPool commandPool, Scene* scene, uint32_t imageIndex);
	void SetupRenderPasses(ShaderManager* shaderManager);
	void TransitionImages(VkQueue graphicsQueue, VkCommandPool commandPool, VkImage swapchainImage);
	void HandleMultiViewportRendering();
	int CreateRenderCommandBuffers();
	int InitSyncObjects();
	int GetQueues();

	void CleanupDepthImage();

	RenderPassManager m_renderPassManager;

	// Vulkan objects
	vkb::Device& m_device;
	vkb::DispatchTable* m_disp = nullptr;
	VmaAllocator m_allocator = VK_NULL_HANDLE;
	VkSurfaceKHR* m_surface = nullptr;
	VulkanDebugUtils* m_debugUtils = nullptr;
	vkb::Swapchain m_swapchain;
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

	// Depth buffer
	VkImage m_depthImage = VK_NULL_HANDLE;
	VkImageView m_depthImageView = VK_NULL_HANDLE;
	VmaAllocation m_depthImageAllocation = VK_NULL_HANDLE;

	// Other resources
	ModelManager* m_modelManager = nullptr;
	MaterialManager* m_materialManager = nullptr;
	DescriptorManager* m_descriptorManager = nullptr;
	VkCommandPool m_commandPool = VK_NULL_HANDLE;
};
