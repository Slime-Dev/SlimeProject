#pragma once
#include <vulkan/vulkan_core.h>

#include "DescriptorManager.h"
#include "vk_mem_alloc.h"

class VulkanDebugUtils;

namespace vkb
{
	struct DispatchTable;
	struct Swapchain;
} // namespace vkb

class Scene;
class Engine;
class DescriptorManager;
class ModelManager;

class Renderer
{
public:
	Renderer() = default;
	~Renderer() = default;

	void SetupViewportAndScissor(vkb::Swapchain swapchain, vkb::DispatchTable disp, VkCommandBuffer& cmd);
	void DrawModels(vkb::DispatchTable disp, VulkanDebugUtils& debugUtils, VmaAllocator allocator, VkCommandBuffer& cmd, ModelManager& modelManager, DescriptorManager& descriptorManager, Scene& scene);
	void CleanUp(VmaAllocator allocator);

private:
	MVP m_mvp; // Push Constant

	ShaderDebug m_shaderDebug;
	VkBuffer m_shaderDebugBuffer = VK_NULL_HANDLE;
	VmaAllocation m_shaderDebugAllocation = VK_NULL_HANDLE;
};
