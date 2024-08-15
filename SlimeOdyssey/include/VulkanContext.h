//
// Created by alexm on 3/07/24.
//

#pragma once

#include <imgui.h>
#include <vector>
#include <vk_mem_alloc.h>

#include "VulkanDebugUtils.h"

struct MaterialManager;
struct TempMaterialTextures;
struct PipelineContainer;
class SlimeWindow;
class Renderer;
class ShaderManager;
class ModelManager;
class Scene;

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
	VkCommandPool GetCommandPool() const;
	VmaAllocator GetAllocator() const;
	vkb::DispatchTable& GetDispatchTable();
	MaterialManager* GetMaterialManager();

	// Helper methods
	int CreateSwapchain(SlimeWindow* window);

private:
	// Initialization methods
	int DeviceInit(SlimeWindow* window);
	int CreateCommandPool();
	int InitImGui(SlimeWindow* window);

	// Vulkan core
	vkb::Instance m_instance;
	vkb::InstanceDispatchTable m_instDisp;
	vkb::DispatchTable m_disp;
	VkSurfaceKHR m_surface{};
	vkb::Device m_device;

	VmaAllocator m_allocator{};
	VkCommandPool m_commandPool = VK_NULL_HANDLE;
	VkDescriptorPool m_imguiDescriptorPool = VK_NULL_HANDLE;

	VulkanDebugUtils m_debugUtils;

	Renderer* m_renderer = nullptr;
	ShaderManager* m_shaderManager = nullptr;
	DescriptorManager* m_descriptorManager = nullptr;
	MaterialManager* m_materialManager = nullptr;

	// Safety checks
	bool m_cleanUpFinished = false;
};
