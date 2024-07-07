//
// Created by alexm on 3/07/24.
//

#pragma once
#include "Camera.h"
#include "PipelineGenerator.h"
#include "ShaderManager.h"
#include "ModelManager.h"
#include "DescriptorManager.h"
#include "SlimeWindow.h"
#include <vector>
#include "Light.h" // TODO Find better place
#include "VulkanDebugUtils.h"

#include <VkBootstrap.h>
#include <map>
#include <memory>
#include <ResourcePathManager.h>
#include <vk_mem_alloc.h>

struct GLFWwindow;

class Engine
{
public:
	Engine(const char* name, int width, int height, bool resizable = false);
	~Engine();

	struct RenderData
	{
		VkQueue graphicsQueue;
		VkQueue presentQueue;

		std::vector<VkImage> swapchainImages;
		std::vector<VkImageView> swapchainImageViews;

		std::map<std::string, std::unique_ptr<PipelineGenerator>> pipelines;

		VkCommandPool commandPool;
		std::vector<VkCommandBuffer> renderCommandBuffers;

		std::vector<VkSemaphore> availableSemaphores;
		std::vector<VkSemaphore> finishedSemaphore;
		std::vector<VkFence> inFlightFences;
		std::vector<VkFence> imageInFlight;
		size_t currentFrame = 0;
	};

	int CreateEngine();
	int SetupManagers();

	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer command_buffer);
	int DeviceInit();
	int CreateSwapchain();
	int GetQueues();
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	int CreateCommandPool();
	int CreateRenderCommandBuffers();

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer& buffer, VmaAllocation& allocation);
	template <class T> void CopyStructToBuffer(T& data, VkBuffer buffer, VmaAllocation allocation);

	void SetupViewportAndScissor(VkCommandBuffer& cmd);

	int InitSyncObjects();

	int Draw(VkCommandBuffer& cmd, int imageIndex);
	int RenderFrame();

	int Cleanup();

	Window& GetWindow() { return m_window; }

	ShaderManager& GetShaderManager() { return m_shaderManager; }
	ModelManager& GetModelManager() { return m_modelManager; }
	ResourcePathManager& GetPathManager() { return m_pathManager; }
	DescriptorManager& GetDescriptorManager() { return m_descriptorManager; }
	VulkanDebugUtils& GetDebugUtils() { return m_debugUtils; }
	Camera& GetCamera() { return m_camera; }

	std::map<std::string, std::unique_ptr<PipelineGenerator>>& GetPipelines() { return data.pipelines; }
	VkDevice GetDevice() const { return m_device.device; }
	VkQueue GetGraphicsQueue() const { return data.graphicsQueue; }
	VkQueue GetPresentQueue() const { return data.presentQueue; }
	VkCommandPool GetCommandPool() const { return data.commandPool; }
	VmaAllocator GetAllocator() const { return m_allocator; }

	RenderData data; // Needs to be removed from here

private:
	void SetupDepthTestingAndLineWidth(VkCommandBuffer& cmd);
	void DrawModels(VkCommandBuffer& cmd);
	int BeginCommandBuffer(VkCommandBuffer& cmd);
	int EndCommandBuffer(VkCommandBuffer& cmd);

	Window m_window;

	vkb::Instance m_instance;
	vkb::InstanceDispatchTable m_instDisp;
	VkSurfaceKHR m_surface{};
	vkb::Device m_device;
	vkb::DispatchTable m_disp;
	vkb::Swapchain m_swapchain;
	VmaAllocator m_allocator{};
	ShaderManager m_shaderManager;
	ModelManager m_modelManager;
	ResourcePathManager m_pathManager;
	DescriptorManager m_descriptorManager;

	VulkanDebugUtils m_debugUtils;

	Camera m_camera = Camera(90.0f, 800.0f / 600.0f, 0.1f, 100.0f);
	MVP m_mvp;

	// TODO Find better place for lights
	const uint8_t MAX_LIGHTS = 1;
	std::vector<LightObject> m_lights;
};