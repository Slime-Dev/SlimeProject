#pragma once
#include "Camera.h"
#include "Scene.h"
#include "VkBootstrap.h"

class VulkanDebugUtils;
class ShaderManager;
class RenderPassManager;

class RenderPassBase
{
public:
	virtual ~RenderPassBase() = default;

	virtual void Setup(vkb::DispatchTable& disp, VmaAllocator allocator, vkb::Swapchain swapchain, ShaderManager* shaderManager, VulkanDebugUtils& debugUtils) = 0;
	virtual void Cleanup(vkb::DispatchTable& disp, VmaAllocator allocator) = 0;
	virtual void Execute(vkb::DispatchTable& disp, VkCommandBuffer& cmd, vkb::Swapchain swapchain, Scene* scene, Camera* camera, RenderPassManager* renderPassManager) = 0;
	virtual VkRenderingInfo* GetRenderingInfo(vkb::Swapchain swapchain, VkImageView& swapchainImageView, VkImageView& depthImageView) = 0;
	virtual void ImGuiDraw(vkb::DispatchTable disp){};

	std::string name;
};
