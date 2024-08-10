//
// Created by alexm on 3/07/24.
//

#include "VulkanContext.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <string>

#define VMA_VULKAN_VERSION 1003000 // Vulkan 1.3
#define VMA_IMPLEMENTATION

#include <vk_mem_alloc.h>

#include "DescriptorManager.h"
#include "ModelManager.h"
#include "PipelineGenerator.h"
#include "Scene.h"
#include "VulkanUtil.h"

#define MAX_FRAMES_IN_FLIGHT 2

// IMGUI
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
#include "ResourcePathManager.h"
#include "ShaderManager.h"
#include "MaterialManager.h"

VulkanContext::~VulkanContext()
{
	if (!m_cleanUpFinished)
		spdlog::error("CLEANUP WAS NOT CALLED ON THE VULKAN CONTEXT!");
}

int VulkanContext::CreateContext(SlimeWindow* window, ModelManager* modelManager)
{
	if (DeviceInit(window) != 0)
		return -1;
	if (CreateCommandPool() != 0)
		return -1;
	if (GetQueues() != 0)
		return -1;
	if (CreateSwapchain(window) != 0)
		return -1;

	m_shaderManager = new ShaderManager();
	m_descriptorManager = new DescriptorManager(m_disp);
	m_materialManager = new MaterialManager(m_disp, m_allocator, m_descriptorManager, m_graphicsQueue, m_commandPool);
	m_renderer.SetUp(&m_disp, m_allocator, m_swapchain, &m_debugUtils, m_shaderManager, m_materialManager, modelManager, m_descriptorManager, m_commandPool, m_graphicsQueue);

	if (CreateRenderCommandBuffers() != 0)
		return -1;
	if (InitSyncObjects() != 0)
		return -1;
	if (InitImGui(window) != 0) // Add this line
		return -1;

	return 0;
}

int VulkanContext::DeviceInit(SlimeWindow* window)
{
	spdlog::debug("Initializing Vulkan...");

	// set up the debug messenger to use spdlog
	auto debugCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) -> VkBool32
	{
		switch (messageSeverity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				spdlog::debug("{}", pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				spdlog::debug("{}", pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				spdlog::warn("{}\n", pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				spdlog::error("{}\n", pCallbackData->pMessage);
				break;
			default:
				spdlog::error("Unknown message severity: {}", pCallbackData->pMessage);
		}

		return VK_FALSE;
	};

	// Create instance with VK_KHR_dynamic_rendering extension
	spdlog::debug("Creating Vulkan instance...");
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,       //
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, //
	};

	vkb::InstanceBuilder instanceBuilder;
	instanceBuilder.enable_extensions(deviceExtensions);
	instanceBuilder.request_validation_layers(true);
	instanceBuilder.set_debug_messenger_severity(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
	instanceBuilder.set_debug_callback(debugCallback);
	instanceBuilder.require_api_version(1, 3, 0);
	vkb::Result<vkb::Instance> instance_ret = instanceBuilder.build();

	if (!instance_ret)
	{
		spdlog::error("Failed to create instance: {}", instance_ret.error().message());
		return -1;
	}
	spdlog::debug("Vulkan instance created.");

	m_instance = instance_ret.value();
	m_instDisp = m_instance.make_table();

	// Create the window surface
	if (VkResult err = glfwCreateWindowSurface(m_instance.instance, window->GetGLFWWindow(), nullptr, &m_surface))
	{
		const char* error_msg;
		int ret = glfwGetError(&error_msg);
		if (ret != 0)
		{
			spdlog::error("GLFW error: {}", error_msg);
		}

		throw std::runtime_error("Failed to create window surface");
	}

	// Select physical device //
	spdlog::debug("Selecting physical device...");

	// Enable VK_EXT_extended_dynamic_state3 features
	VkPhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3Features = {};
	extendedDynamicState3Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
	extendedDynamicState3Features.extendedDynamicState3ColorBlendEnable = VK_TRUE;
	extendedDynamicState3Features.extendedDynamicState3ColorBlendEquation = VK_TRUE;
	extendedDynamicState3Features.extendedDynamicState3ColorWriteMask = VK_TRUE;
	extendedDynamicState3Features.extendedDynamicState3RasterizationSamples = VK_TRUE;
	extendedDynamicState3Features.extendedDynamicState3SampleMask = VK_TRUE;
	extendedDynamicState3Features.extendedDynamicState3AlphaToCoverageEnable = VK_TRUE;
	extendedDynamicState3Features.extendedDynamicState3AlphaToOneEnable = VK_TRUE;
	extendedDynamicState3Features.extendedDynamicState3LogicOpEnable = VK_TRUE;
	extendedDynamicState3Features.extendedDynamicState3ColorBlendAdvanced = VK_TRUE;

	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures = {};
	extendedDynamicStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
	extendedDynamicStateFeatures.extendedDynamicState = VK_TRUE;
	extendedDynamicStateFeatures.pNext = &extendedDynamicState3Features;

	// Enable mesh shader features
	VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures = {};
	meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
	meshShaderFeatures.taskShader = VK_TRUE;
	meshShaderFeatures.meshShader = VK_TRUE;
	meshShaderFeatures.multiviewMeshShader = VK_TRUE;
	meshShaderFeatures.primitiveFragmentShadingRateMeshShader = VK_TRUE;
	meshShaderFeatures.meshShaderQueries = VK_TRUE;
	meshShaderFeatures.pNext = &extendedDynamicState3Features;

	// Enable Vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features13 = {};
	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.dynamicRendering = VK_TRUE;
	features13.synchronization2 = VK_TRUE;
	features13.maintenance4 = VK_TRUE;
	features13.pNext = &meshShaderFeatures;

	// Enable Vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12 = {};
	features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features12.bufferDeviceAddress = VK_TRUE;
	features12.descriptorIndexing = VK_TRUE;
	features12.pNext = &features13;

	// Enable Vulkan 1.1 features
	VkPhysicalDeviceVulkan11Features features11 = {};
	features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	features11.multiview = VK_TRUE;
	features11.pNext = &features12;

	// Set up basic device features
	VkPhysicalDeviceFeatures2 features2 = {};
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features2.features.fillModeNonSolid = VK_TRUE;
	features2.features.wideLines = VK_TRUE;
	features2.features.geometryShader = VK_TRUE;
	features2.pNext = &features11;

	vkb::PhysicalDeviceSelector phys_device_selector(m_instance);
	auto phys_device_ret = phys_device_selector.set_minimum_version(1, 3)
	                               .add_required_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME)
	                               .add_required_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME)
	                               .add_required_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME)
	                               .add_required_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME)
	                               .set_required_features_11(features11)
	                               .set_required_features_12(features12)
	                               .set_required_features_13(features13)
	                               .set_required_features(features2.features)
	                               .set_surface(m_surface)
	                               .select();
	if (!phys_device_ret)
	{
		spdlog::error("Failed to select physical device: {}", phys_device_ret.error().message());
		return -1;
	}
	spdlog::debug("Physical device selected.");
	spdlog::debug("Physical device: {}", phys_device_ret.value().properties.deviceName);

	vkb::PhysicalDevice physical_device = phys_device_ret.value();

	// Create device (extensions are automatically enabled as they were required in PhysicalDeviceSelector)
	spdlog::debug("Creating logical device...");
	vkb::DeviceBuilder device_builder{ physical_device };
	device_builder.add_pNext(&extendedDynamicStateFeatures);
	auto device_ret = device_builder.build();

	if (!device_ret)
	{
		spdlog::error("Failed to create logical device: {}", device_ret.error().message());
		return -1;
	}
	spdlog::debug("Logical device created.");
	m_device = device_ret.value();

	m_disp = m_device.make_table();

	// Setup the VMA allocator
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = physical_device.physical_device;
	allocatorInfo.device = m_device.device;
	allocatorInfo.instance = m_instance.instance;

	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = m_instance.fp_vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = m_device.fp_vkGetDeviceProcAddr;

	// Instance functions
	vulkanFunctions.vkGetPhysicalDeviceProperties = m_instDisp.fp_vkGetPhysicalDeviceProperties;
	vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = m_instDisp.fp_vkGetPhysicalDeviceMemoryProperties;

	// Device functions
	vulkanFunctions.vkAllocateMemory = m_disp.fp_vkAllocateMemory;
	vulkanFunctions.vkFreeMemory = m_disp.fp_vkFreeMemory;
	vulkanFunctions.vkMapMemory = m_disp.fp_vkMapMemory;
	vulkanFunctions.vkUnmapMemory = m_disp.fp_vkUnmapMemory;
	vulkanFunctions.vkFlushMappedMemoryRanges = m_disp.fp_vkFlushMappedMemoryRanges;
	vulkanFunctions.vkInvalidateMappedMemoryRanges = m_disp.fp_vkInvalidateMappedMemoryRanges;
	vulkanFunctions.vkBindBufferMemory = m_disp.fp_vkBindBufferMemory;
	vulkanFunctions.vkBindImageMemory = m_disp.fp_vkBindImageMemory;
	vulkanFunctions.vkGetBufferMemoryRequirements = m_disp.fp_vkGetBufferMemoryRequirements;
	vulkanFunctions.vkGetImageMemoryRequirements = m_disp.fp_vkGetImageMemoryRequirements;
	vulkanFunctions.vkCreateBuffer = m_disp.fp_vkCreateBuffer;
	vulkanFunctions.vkDestroyBuffer = m_disp.fp_vkDestroyBuffer;
	vulkanFunctions.vkCreateImage = m_disp.fp_vkCreateImage;
	vulkanFunctions.vkDestroyImage = m_disp.fp_vkDestroyImage;
	vulkanFunctions.vkCmdCopyBuffer = m_disp.fp_vkCmdCopyBuffer;

	allocatorInfo.pVulkanFunctions = &vulkanFunctions;

	VK_CHECK(vmaCreateAllocator(&allocatorInfo, &m_allocator));

	m_debugUtils = VulkanDebugUtils(m_instDisp, m_device);

	m_debugUtils.SetObjectName(m_device.device, "MainDevice");

	return 0;
}

int VulkanContext::CreateSwapchain(SlimeWindow* window)
{
	spdlog::debug("Creating swapchain...");
	m_disp.deviceWaitIdle();

	vkb::SwapchainBuilder swapchain_builder(m_device, m_surface);

	auto swap_ret = swapchain_builder.use_default_format_selection()
	                        .set_desired_format(VkSurfaceFormatKHR{ .format = VK_FORMAT_B8G8R8A8_UNORM, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
	                        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // Use vsync present mode
	                        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	                        .set_old_swapchain(m_swapchain)
	                        .set_desired_extent(window->GetWidth(), window->GetHeight())
	                        .build();

	if (!swap_ret)
	{
		spdlog::error("Failed to create swapchain: {}", swap_ret.error().message());
		return -1;
	}

	vkb::destroy_swapchain(m_swapchain);
	m_swapchain = swap_ret.value();

	// Delete old image views
	for (auto& image_view: m_swapchainImageViews)
	{
		m_disp.destroyImageView(image_view, nullptr);
	}

	m_swapchainImages = m_swapchain.get_images().value();
	m_swapchainImageViews = m_swapchain.get_image_views().value();

	for (size_t i = 0; i < m_swapchainImages.size(); i++)
	{
		m_debugUtils.SetObjectName(m_swapchainImages[i], "SwapchainImage_" + std::to_string(i));
		m_debugUtils.SetObjectName(m_swapchainImageViews[i], "SwapchainImageView_" + std::to_string(i));
	}

	return 0;
}

int VulkanContext::GetQueues()
{
	auto gq = m_device.get_queue(vkb::QueueType::graphics);
	if (!gq.has_value())
	{
		spdlog::error("failed to get graphics queue: {}", gq.error().message());
		return -1;
	}
	m_graphicsQueue = gq.value();

	auto pq = m_device.get_queue(vkb::QueueType::present);
	if (!pq.has_value())
	{
		spdlog::error("failed to get present queue: {}", pq.error().message());
		return -1;
	}
	m_presentQueue = pq.value();

	m_debugUtils.SetObjectName(m_graphicsQueue, "GraphicsQueue");
	m_debugUtils.SetObjectName(m_presentQueue, "PresentQueue");
	return 0;
}

int VulkanContext::CreateCommandPool()
{
	spdlog::debug("Creating command pool...");
	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = m_device.get_queue_index(vkb::QueueType::graphics).value();
	// Assuming graphics queue family
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VK_CHECK(m_disp.createCommandPool(&pool_info, nullptr, &m_commandPool));

	return 0;
}

int VulkanContext::CreateRenderCommandBuffers()
{
	spdlog::debug("Recording command buffers...");
	m_renderCommandBuffers.resize(m_swapchain.image_count + 10);

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = m_commandPool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = static_cast<uint32_t>(m_renderCommandBuffers.size());

	VK_CHECK(m_disp.allocateCommandBuffers(&alloc_info, m_renderCommandBuffers.data()));

	int index = 0;
	for (auto& cmd: m_renderCommandBuffers)
	{
		m_debugUtils.SetObjectName(cmd, std::string("Render Command Buffer: " + std::to_string(index++)));
	}

	return 0;
}

int VulkanContext::InitSyncObjects()
{
	spdlog::debug("Initializing synchronization objects...");
	// Create synchronization objects
	m_availableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_finishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_imageInFlight.resize(m_swapchain.image_count, VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VK_CHECK(m_disp.createSemaphore(&semaphore_info, nullptr, &m_availableSemaphores[i]));
		VK_CHECK(m_disp.createSemaphore(&semaphore_info, nullptr, &m_finishedSemaphore[i]));
		VK_CHECK(m_disp.createFence(&fence_info, nullptr, &m_inFlightFences[i]));
	}

	return 0;
}

void SetupImGuiStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	// Modern color palette with darker greys and green accent
	ImVec4 bgDark = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	ImVec4 bgMid = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	ImVec4 bgLight = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	ImVec4 accent = ImVec4(0.10f, 0.60f, 0.30f, 1.00f);
	ImVec4 accentLight = ImVec4(0.20f, 0.70f, 0.40f, 1.00f);
	ImVec4 textPrimary = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
	ImVec4 textSecondary = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);

	// Set colors
	colors[ImGuiCol_Text] = textPrimary;
	colors[ImGuiCol_TextDisabled] = textSecondary;
	colors[ImGuiCol_WindowBg] = bgDark;
	colors[ImGuiCol_ChildBg] = bgMid;
	colors[ImGuiCol_PopupBg] = bgMid;
	colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = bgLight;
	colors[ImGuiCol_FrameBgHovered] = accent;
	colors[ImGuiCol_FrameBgActive] = accentLight;
	colors[ImGuiCol_TitleBg] = bgMid;
	colors[ImGuiCol_TitleBgActive] = accent;
	colors[ImGuiCol_TitleBgCollapsed] = bgDark;
	colors[ImGuiCol_MenuBarBg] = bgMid;
	colors[ImGuiCol_ScrollbarBg] = bgDark;
	colors[ImGuiCol_ScrollbarGrab] = bgLight;
	colors[ImGuiCol_ScrollbarGrabHovered] = accent;
	colors[ImGuiCol_ScrollbarGrabActive] = accentLight;
	colors[ImGuiCol_CheckMark] = accentLight;
	colors[ImGuiCol_SliderGrab] = accent;
	colors[ImGuiCol_SliderGrabActive] = accentLight;
	colors[ImGuiCol_Button] = bgLight;
	colors[ImGuiCol_ButtonHovered] = accent;
	colors[ImGuiCol_ButtonActive] = accentLight;
	colors[ImGuiCol_Header] = bgLight;
	colors[ImGuiCol_HeaderHovered] = accent;
	colors[ImGuiCol_HeaderActive] = accentLight;
	colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = accent;
	colors[ImGuiCol_SeparatorActive] = accentLight;
	colors[ImGuiCol_ResizeGrip] = bgLight;
	colors[ImGuiCol_ResizeGripHovered] = accent;
	colors[ImGuiCol_ResizeGripActive] = accentLight;
	colors[ImGuiCol_Tab] = bgMid;
	colors[ImGuiCol_TabHovered] = accent;
	colors[ImGuiCol_TabActive] = accentLight;
	colors[ImGuiCol_TabUnfocused] = bgDark;
	colors[ImGuiCol_TabUnfocusedActive] = bgLight;
	colors[ImGuiCol_TabSelectedOverline] = bgLight;
	colors[ImGuiCol_PlotLines] = accent;
	colors[ImGuiCol_PlotLinesHovered] = accentLight;
	colors[ImGuiCol_PlotHistogram] = accent;
	colors[ImGuiCol_PlotHistogramHovered] = accentLight;
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.80f, 0.50f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.20f, 0.80f, 0.50f, 0.90f);
	colors[ImGuiCol_NavHighlight] = accent;
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.20f, 0.80f, 0.50f, 0.90f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

	// Adjust style properties
	style.WindowPadding = ImVec2(10, 10);
	style.FramePadding = ImVec2(8, 4);
	style.ItemSpacing = ImVec2(10, 8);
	style.ItemInnerSpacing = ImVec2(8, 6);
	style.IndentSpacing = 25.0f;
	style.ScrollbarSize = 12.0f;
	style.GrabMinSize = 12.0f;

	style.WindowBorderSize = 1.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupBorderSize = 1.0f;
	style.FrameBorderSize = 0.0f;

	style.WindowRounding = 6.0f;
	style.ChildRounding = 6.0f;
	style.FrameRounding = 4.0f;
	style.PopupRounding = 6.0f;
	style.ScrollbarRounding = 6.0f;
	style.GrabRounding = 4.0f;
	style.TabRounding = 4.0f;

	// Font settings
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF(ResourcePathManager::GetFontPath("JetBrainsMono-Regular.ttf").c_str(), 16.0f);

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
}

int VulkanContext::InitImGui(SlimeWindow* window)
{
	// Create Descriptor Pool for ImGui
	VkDescriptorPoolSize poolSizes[] = {
		{		       VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
		{         VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
		{         VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
		{  VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
		{  VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
		{        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
		{        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
		{      VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
	};

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 1000;
	poolInfo.poolSizeCount = std::size(poolSizes);
	poolInfo.pPoolSizes = poolSizes;

	VK_CHECK(m_disp.createDescriptorPool(&poolInfo, nullptr, &m_imguiDescriptorPool));

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void) io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	SetupImGuiStyle();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForVulkan(window->GetGLFWWindow(), true);
	// Load Vulkan functions
	ImGui_ImplVulkan_LoadFunctions(
	        [](const char* function_name, void* user_data)
	        {
		        VulkanContext* context = static_cast<VulkanContext*>(user_data);

		        // Map KHR variants to core functions
		        if (strcmp(function_name, "vkCmdBeginRenderingKHR") == 0)
			        return context->m_instDisp.getInstanceProcAddr("vkCmdBeginRendering");
		        if (strcmp(function_name, "vkCmdEndRenderingKHR") == 0)
			        return context->m_instDisp.getInstanceProcAddr("vkCmdEndRendering");

		        return context->m_instDisp.getInstanceProcAddr(function_name);
	        },
	        this);

	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = m_instance;
	initInfo.PhysicalDevice = m_device.physical_device;
	initInfo.Device = m_device.device;
	initInfo.QueueFamily = m_device.get_queue_index(vkb::QueueType::graphics).value();
	initInfo.Queue = m_graphicsQueue;
	initInfo.PipelineCache = VK_NULL_HANDLE;
	initInfo.DescriptorPool = m_imguiDescriptorPool;
	initInfo.Subpass = 0;
	initInfo.MinImageCount = m_swapchain.image_count;
	initInfo.ImageCount = m_swapchain.image_count;
	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	initInfo.Allocator = nullptr;
	initInfo.CheckVkResultFn = nullptr;
	initInfo.UseDynamicRendering = true;

	VkPipelineRenderingCreateInfoKHR imguiPipelineInfo = {};
	imguiPipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	imguiPipelineInfo.pNext = nullptr;
	imguiPipelineInfo.viewMask = 0;
	imguiPipelineInfo.colorAttachmentCount = 1;
	imguiPipelineInfo.pColorAttachmentFormats = &m_swapchain.image_format;
	imguiPipelineInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
	imguiPipelineInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
	initInfo.PipelineRenderingCreateInfo = imguiPipelineInfo;

	ImGui_ImplVulkan_Init(&initInfo);

	// Upload Fonts
	{
		ImGui_ImplVulkan_CreateFontsTexture();
	}

	return 0;
}

int VulkanContext::RenderFrame(ModelManager& modelManager, SlimeWindow* window, Scene* scene)
{
	if (window->WindowSuspended())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		return 0;
	}

	// Wait for the frame to be finished
	VK_CHECK(m_disp.waitForFences(1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX));

	uint32_t imageIndex;
	VkResult result = m_disp.acquireNextImageKHR(m_swapchain, UINT64_MAX, m_availableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		if (CreateSwapchain(window) != 0)
			return -1;
		return 0;
	}
	else
	{
		VK_CHECK(result);
	}

	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (m_imageInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		m_disp.waitForFences(1, &m_imageInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	// Mark the image as now being in use by this frame
	m_imageInFlight[imageIndex] = m_inFlightFences[m_currentFrame];

	// Begin command buffer recording
	VkCommandBuffer cmd = m_renderCommandBuffers[imageIndex];

	if (m_renderer.Draw(cmd, m_commandPool, m_graphicsQueue, m_swapchainImages, m_swapchainImageViews, imageIndex, scene) != 0)
		return -1;

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore wait_semaphores[] = { m_availableSemaphores[m_currentFrame] };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd;

	VkSemaphore signal_semaphores[] = { m_finishedSemaphore[m_currentFrame] };
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;

	m_disp.resetFences(1, &m_inFlightFences[m_currentFrame]);

	m_debugUtils.BeginQueueDebugMarker(m_graphicsQueue, "FrameSubmission", debugUtil_FrameSubmission);
	VK_CHECK(m_disp.queueSubmit(m_graphicsQueue, 1, &submit_info, m_inFlightFences[m_currentFrame]));
	m_debugUtils.EndQueueDebugMarker(m_graphicsQueue);

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphores;

	VkSwapchainKHR swapchains[] = { m_swapchain };
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swapchains;
	present_info.pImageIndices = &imageIndex;

	result = m_disp.queuePresentKHR(m_presentQueue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		if (CreateSwapchain(window) != 0)
			return -1;
	}
	else
	{
		VK_CHECK(result);
	}

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	if (window->ShouldClose())
	{
		m_disp.deviceWaitIdle();
	}

	return 0;
}

int VulkanContext::Cleanup(ModelManager& modelManager)
{
	spdlog::debug("Cleaning up...");

	m_disp.deviceWaitIdle();

	// Cleanup ImGui
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// Destroy ImGui descriptor pool
	m_disp.destroyDescriptorPool(m_imguiDescriptorPool, nullptr);

	// Destroy synchronization objects
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_disp.destroySemaphore(m_availableSemaphores[i], nullptr);
		m_disp.destroySemaphore(m_finishedSemaphore[i], nullptr);
		m_disp.destroyFence(m_inFlightFences[i], nullptr);
	}

	for (auto& image_view: m_swapchainImageViews)
	{
		m_disp.destroyImageView(image_view, nullptr);
	}

	modelManager.UnloadAllResources(m_disp, m_allocator);

	m_shaderManager->CleanUp(m_disp);
	delete m_shaderManager;

	m_descriptorManager->Cleanup();
	delete m_descriptorManager;

	delete m_materialManager;

	m_renderer.CleanUp();

	m_disp.destroyCommandPool(m_commandPool, nullptr);

	vkb::destroy_swapchain(m_swapchain);
	vkb::destroy_surface(m_instance, m_surface);

	// Destroy the allocator
	vmaDestroyAllocator(m_allocator);

	vkb::destroy_device(m_device);
	vkb::destroy_instance(m_instance);

	m_cleanUpFinished = true;

	return 0;
}

// GETTERS //

ShaderManager* VulkanContext::GetShaderManager()
{
	return m_shaderManager;
}

DescriptorManager* VulkanContext::GetDescriptorManager()
{
	return m_descriptorManager;
}

VulkanDebugUtils& VulkanContext::GetDebugUtils()
{
	return m_debugUtils;
}

VkDevice VulkanContext::GetDevice() const
{
	return m_device.device;
}

const vkb::Swapchain& VulkanContext::GetSwapchain() const
{
	return m_swapchain;
}

VkQueue VulkanContext::GetGraphicsQueue() const
{
	return m_graphicsQueue;
}

VkQueue VulkanContext::GetPresentQueue() const
{
	return m_presentQueue;
}

VkCommandPool VulkanContext::GetCommandPool() const
{
	return m_commandPool;
}

VmaAllocator VulkanContext::GetAllocator() const
{
	return m_allocator;
}

vkb::DispatchTable& VulkanContext::GetDispatchTable()
{
	return m_disp;
}

MaterialManager* VulkanContext::GetMaterialManager()
{
	return m_materialManager;
}
