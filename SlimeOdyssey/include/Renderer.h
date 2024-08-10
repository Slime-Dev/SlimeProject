#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "RenderPassManager.h"

// Forward declarations
namespace vkb
{
	struct DispatchTable;
	struct Swapchain;
} // namespace vkb
class VulkanDebugUtils;
class ModelManager;
class DescriptorManager;
class Scene;
class MaterialManager;

class Renderer
{
public:
	Renderer() = default;
	~Renderer() = default;

	void SetUp(vkb::DispatchTable* disp, VmaAllocator allocator, vkb::Swapchain swapchain, VulkanDebugUtils* debugUtils, ShaderManager* shaderManager, MaterialManager* materialManager, ModelManager* modelManager, DescriptorManager* descriptorManager, VkCommandPool commandPool, VkQueue graphicsQueue);
	void CleanUp();

	int Draw(VkCommandBuffer& cmd, VkCommandPool commandPool, VkQueue graphicsQueue, std::vector<VkImage>& swapchainImages, std::vector<VkImageView>& swapchainImageViews, uint32_t imageIndex, Scene* scene);

	void CreateDepthImage();

private:
	void SetupRenderPasses(ShaderManager* shaderManager);
	void TransitionImages(VkQueue graphicsQueue, VkCommandPool commandPool, VkImage swapchainImage);
	void HandleMultiViewportRendering();

	void CleanupDepthImage();

	RenderPassManager m_renderPassManager;

	// Vulkan objects
	vkb::DispatchTable* m_disp = nullptr;
	VmaAllocator m_allocator = VK_NULL_HANDLE;
	vkb::Swapchain m_swapchain;
	VulkanDebugUtils* m_debugUtils = nullptr;

	// Depth buffer
	VkImage m_depthImage = VK_NULL_HANDLE;
	VkImageView m_depthImageView = VK_NULL_HANDLE;
	VmaAllocation m_depthImageAllocation = VK_NULL_HANDLE;

	// Other resources
	ModelManager* m_modelManager = nullptr;
	MaterialManager* m_materialManager = nullptr;
	DescriptorManager* m_descriptorManager = nullptr;
	VkCommandPool m_commandPool = VK_NULL_HANDLE;
	VkQueue m_graphicsQueue = VK_NULL_HANDLE;

	// Render settings
	glm::vec3 m_clearColor = glm::vec3(0.0f, 0.0f, 0.0f);
};
