//
// Created by alexm on 3/07/24.
//

#pragma once

#include <map>
#include <memory>
#include <vector>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>

#include "Camera.h"
#include "DescriptorManager.h"
#include "InputManager.h"
#include "Light.h"
#include "ModelManager.h"
#include "PipelineGenerator.h"
#include "ResourcePathManager.h"
#include "ShaderManager.h"
#include "SlimeWindow.h"
#include "VulkanDebugUtils.h"

struct GLFWwindow;

class Engine
{
public:
	explicit Engine(SlimeWindow* window);
	~Engine();

	int CreateEngine();
	int SetupManagers();
	int RenderFrame();
	int Cleanup();

	// Getters
	SlimeWindow* GetWindow();
	ShaderManager& GetShaderManager();
	ModelManager& GetModelManager();
	ResourcePathManager& GetPathManager();
	DescriptorManager& GetDescriptorManager();
	VulkanDebugUtils& GetDebugUtils();
	Camera& GetCamera();
	InputManager* GetInputManager();
	std::map<std::string, PipelineContainer>& GetPipelines();
	VkDevice GetDevice() const;
	VkQueue GetGraphicsQueue() const;
	VkQueue GetPresentQueue() const;
	VkCommandPool GetCommandPool() const;
	VmaAllocator GetAllocator() const;

	// Helper methods
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer& buffer, VmaAllocation& allocation);
	int BeginCommandBuffer(VkCommandBuffer& cmd);
	int EndCommandBuffer(VkCommandBuffer& cmd);
	int CreateSwapchain(); // Needs to be public for window resize callback
	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer command_buffer);

private:
	// Initialization methods
	int DeviceInit();
	int GetQueues();
	int CreateCommandPool();
	int CreateRenderCommandBuffers();
	int InitSyncObjects();

	// Rendering methods
	int Draw(VkCommandBuffer& cmd, int imageIndex);
	void SetupViewportAndScissor(VkCommandBuffer& cmd);
	void SetupDepthTestingAndLineWidth(VkCommandBuffer& cmd);
	void DrawModels(VkCommandBuffer& cmd);

	// Utility methods
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	template<class T>
	void CopyStructToBuffer(T& data, VkBuffer buffer, VmaAllocation allocation);

	// Structs
	struct TempMaterialTextures
	{
		const ModelManager::TextureResource* albedo;
		VkSampler albedoSampler;
		const ModelManager::TextureResource* normal;
		VkSampler normalSampler;
		const ModelManager::TextureResource* metallic;
		VkSampler metallicSampler;
		const ModelManager::TextureResource* roughness;
		VkSampler roughnessSampler;
		const ModelManager::TextureResource* ao;
		VkSampler aoSampler;
	};

	struct RenderData
	{
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue presentQueue = VK_NULL_HANDLE;
		std::vector<VkImage> swapchainImages;
		std::vector<VkImageView> swapchainImageViews;
		VmaAllocation depthImageAllocation;
		VkImage depthImage = VK_NULL_HANDLE;
		VkImageView depthImageView = VK_NULL_HANDLE;
		std::map<std::string, PipelineContainer> pipelines;
		VkCommandPool commandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> renderCommandBuffers;
		std::vector<VkSemaphore> availableSemaphores;
		std::vector<VkSemaphore> finishedSemaphore;
		std::vector<VkFence> inFlightFences;
		std::vector<VkFence> imageInFlight;
		size_t currentFrame = 0;
	};

	// Core engine components
	SlimeWindow* m_window = nullptr;
	InputManager* m_inputManager = nullptr;
	Camera m_camera;
	const uint8_t MAX_LIGHTS = 1;
	std::vector<LightObject> m_lights;

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

	// Resource managers
	ShaderManager m_shaderManager;
	ModelManager m_modelManager;
	ResourcePathManager m_pathManager;
	DescriptorManager m_descriptorManager;
	VulkanDebugUtils m_debugUtils;

	// Buffers and allocations
	MVP m_mvp;
	VkBuffer m_mvpBuffer;
	VmaAllocation m_mvpAllocation;
	VkBuffer materialBuffer;
	VmaAllocation materialAllocation;
	VkBuffer cameraUBOBBuffer;
	VmaAllocation cameraUBOAllocation;
	VkBuffer LightBuffer;
	VmaAllocation LightAllocation;

	// Textures
	TempMaterialTextures m_tempMaterialTextures;
};
