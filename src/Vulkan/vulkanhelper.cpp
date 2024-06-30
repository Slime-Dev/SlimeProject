#include "vulkanhelper.h"
#include <spdlog/spdlog.h>
#include "fileHelper.h"
#include "vulkanwindow.h"
#include <string>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>

#define MAX_FRAMES_IN_FLIGHT 2

namespace SlimeEngine
{
VkCommandBuffer BeginSingleTimeCommands(Init& init, RenderData& data)
{
	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool                 = data.commandPool; // Assuming you have a command pool created
	alloc_info.commandBufferCount          = 1;

	VkCommandBuffer command_buffer;
	if (vkAllocateCommandBuffers(init.device, &alloc_info, &command_buffer) != VK_SUCCESS)
	{
		spdlog::error("Failed to allocate command buffer!");
		return VK_NULL_HANDLE;
	}

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
	{
		spdlog::error("Failed to begin recording command buffer!");
		return VK_NULL_HANDLE;
	}

	return command_buffer;
}

void EndSingleTimeCommands(Init& init, RenderData& data, VkCommandBuffer command_buffer)
{
	if (command_buffer == VK_NULL_HANDLE)
	{
		return;
	}

	if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
	{
		spdlog::error("Failed to record command buffer!");
		return;
	}

	VkSubmitInfo submit_info       = {};
	submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &command_buffer;

	VkQueue graphics_queue = data.graphicsQueue; // Assuming you have a graphics queue defined

	if (vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		spdlog::error("Failed to submit command buffer!");
		return;
	}

	// Wait for the command buffer to finish execution
	if (vkQueueWaitIdle(graphics_queue) != VK_SUCCESS)
	{
		spdlog::error("Failed to wait for queue to finish!");
		return;
	}

	// Free the command buffer
	vkFreeCommandBuffers(init.device, data.commandPool, 1, &command_buffer);
}

int DeviceInit(Init& init)
{
	spdlog::info("Initializing Vulkan...");

	init.window = SlimeEngine::CreateWindow("Vulkan Triangle", true);

	// set up the debug messenger to use spdlog
	auto debugCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) -> VkBool32 {
		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			spdlog::debug("{}", pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			spdlog::info("{}", pCallbackData->pMessage);
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
	spdlog::info("Creating Vulkan instance...");
	vkb::InstanceBuilder instance_builder;
	auto instance_ret = instance_builder
	                    .request_validation_layers(true)
	                    .set_debug_messenger_severity(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	                    .set_debug_callback(debugCallback)
	                    .require_api_version(1, 3, 0)
	                    .build();

	if (!instance_ret)
	{
		spdlog::error("Failed to create instance: {}", instance_ret.error().message());
		return -1;
	}
	spdlog::info("Vulkan instance created.");

	init.instance = instance_ret.value();
	init.instDisp = init.instance.make_table();

	init.surface = SlimeEngine::CreateSurface(init.instance.instance, init.window);

	// Select physical device //
	spdlog::info("Selecting physical device...");
	VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features.dynamicRendering = true;
	features.synchronization2 = true;

	VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing  = true;

	vkb::PhysicalDeviceSelector phys_device_selector(init.instance);

	auto phys_device_ret = phys_device_selector
	                       .set_minimum_version(1, 3)
	                       .set_required_features_13(features)
	                       .set_required_features_12(features12)
	                       .set_surface(init.surface)
	                       .select();

	if (!phys_device_ret)
	{
		spdlog::error("Failed to select physical device: {}", phys_device_ret.error().message());
		return -1;
	}
	spdlog::info("Physical device selected.");
	spdlog::info("Physical device: {}", phys_device_ret.value().properties.deviceName);

	vkb::PhysicalDevice physical_device = phys_device_ret.value();

	// Create device (extension is automatically enabled as it was required in PhysicalDeviceSelector)
	spdlog::info("Creating logical device...");
	vkb::DeviceBuilder device_builder{ physical_device };
	auto device_ret = device_builder.build();

	if (!device_ret)
	{
		spdlog::error("Failed to create logical device: {}", device_ret.error().message());
		return -1;
	}
	spdlog::info("Logical device created.");
	init.device = device_ret.value();

	init.disp = init.device.make_table();

	return 0;
}

int CreateSwapchain(Init& init, RenderData& data)
{
	spdlog::info("Creating swapchain...");
	vkDeviceWaitIdle(init.device);

	vkb::SwapchainBuilder swapchain_builder(init.device, init.surface);

	auto swap_ret = swapchain_builder
	                .use_default_format_selection()
	                .set_desired_format(VkSurfaceFormatKHR{ .format = VK_FORMAT_B8G8R8A8_UNORM, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
	                .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // Use vsync present mode
	                .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	                .set_old_swapchain(init.swapchain)
	                .build();

	if (!swap_ret)
	{
		spdlog::error("Failed to create swapchain: {}", swap_ret.error().message());
		return -1;
	}

	vkb::destroy_swapchain(init.swapchain);
	init.swapchain = swap_ret.value();

	// Delete old image views
	for (auto& image_view : data.swapchainImageViews)
	{
		init.disp.destroyImageView(image_view, nullptr);
	}

	data.swapchainImages     = init.swapchain.get_images().value();
	data.swapchainImageViews = init.swapchain.get_image_views().value();

	return 0;
}

int GetQueues(Init& init, RenderData& data)
{
	auto gq = init.device.get_queue(vkb::QueueType::graphics);
	if (!gq.has_value())
	{
		spdlog::error("failed to get graphics queue: {}", gq.error().message());
		return -1;
	}
	data.graphicsQueue = gq.value();

	auto pq = init.device.get_queue(vkb::QueueType::present);
	if (!pq.has_value())
	{
		spdlog::error("failed to get present queue: {}", pq.error().message());
		return -1;
	}
	data.presentQueue = pq.value();
	return 0;
}

VkShaderModule CreateShaderModule(Init& init, const std::vector<char>& code)
{
	spdlog::info("Creating shader module...");
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize                 = code.size();
	create_info.pCode                    = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (init.disp.createShaderModule(&create_info, nullptr, &shaderModule) != VK_SUCCESS)
	{
		return VK_NULL_HANDLE; // failed to create shader module
	}

	return shaderModule;
}

int CreateGraphicsPipeline(Init& init, RenderData& data, const char* shaderDir)
{
	spdlog::info("Creating graphics pipeline...");
	auto vert_code = SlimeEngine::ReadFile(std::string(shaderDir) + "/shader.vert.spv");
	auto frag_code = SlimeEngine::ReadFile(std::string(shaderDir) + "/shader.frag.spv");

	VkShaderModule vert_module = CreateShaderModule(init, vert_code);
	VkShaderModule frag_module = CreateShaderModule(init, frag_code);
	if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE)
	{
		spdlog::error("Failed to create shader modules!");
		return -1; // failed to create shader modules
	}

	VkPipelineShaderStageCreateInfo vert_stage_info = {};
	vert_stage_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_stage_info.stage                           = VK_SHADER_STAGE_VERTEX_BIT;
	vert_stage_info.module                          = vert_module;
	vert_stage_info.pName                           = "main";

	VkPipelineShaderStageCreateInfo frag_stage_info = {};
	frag_stage_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_stage_info.stage                           = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_stage_info.module                          = frag_module;
	frag_stage_info.pName                           = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vert_stage_info, frag_stage_info };

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount        = 0;
	vertex_input_info.vertexAttributeDescriptionCount      = 0;

	VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
	input_assembly.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable                 = VK_FALSE;

	VkViewport viewport = {};
	viewport.x          = 0.0f;
	viewport.y          = 0.0f;
	viewport.width      = (float)init.swapchain.extent.width;
	viewport.height     = (float)init.swapchain.extent.height;
	viewport.minDepth   = 0.0f;
	viewport.maxDepth   = 1.0f;

	VkRect2D scissor = {};
	scissor.offset   = { 0, 0 };
	scissor.extent   = init.swapchain.extent;

	VkPipelineViewportStateCreateInfo viewport_state = {};
	viewport_state.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount                     = 1;
	viewport_state.pViewports                        = &viewport;
	viewport_state.scissorCount                      = 1;
	viewport_state.pScissors                         = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable                       = VK_FALSE;
	rasterizer.rasterizerDiscardEnable                = VK_FALSE;
	rasterizer.polygonMode                            = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth                              = 1.0f;
	rasterizer.cullMode                               = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace                              = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable                        = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable                  = VK_FALSE;
	multisampling.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	color_blend_attachment.colorWriteMask                      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable                         = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo color_blending = {};
	color_blending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable                       = VK_FALSE;
	color_blending.logicOp                             = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount                     = 1;
	color_blending.pAttachments                        = &color_blend_attachment;
	color_blending.blendConstants[0]                   = 0.0f;
	color_blending.blendConstants[1]                   = 0.0f;
	color_blending.blendConstants[2]                   = 0.0f;
	color_blending.blendConstants[3]                   = 0.0f;

	// Enable dynamic viewport and scissor states
	VkPipelineDynamicStateCreateInfo dynamic_state = {};
	dynamic_state.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	VkDynamicState dynamic_states[]                = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	dynamic_state.dynamicStateCount = 2;
	dynamic_state.pDynamicStates    = dynamic_states;

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount             = 0;
	pipeline_layout_info.pushConstantRangeCount     = 0;

	if (init.disp.createPipelineLayout(&pipeline_layout_info, nullptr, &data.pipelineLayout) != VK_SUCCESS)
	{
		spdlog::error("Failed to create pipeline layout!");
		return -1;
	}

	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount                   = 2;
	pipeline_info.pStages                      = shader_stages;
	pipeline_info.pVertexInputState            = &vertex_input_info;
	pipeline_info.pInputAssemblyState          = &input_assembly;
	pipeline_info.pViewportState               = &viewport_state;
	pipeline_info.pRasterizationState          = &rasterizer;
	pipeline_info.pMultisampleState            = &multisampling;
	pipeline_info.pColorBlendState             = VK_NULL_HANDLE; // &color_blending;
	pipeline_info.layout                       = data.pipelineLayout;
	pipeline_info.renderPass                   = VK_NULL_HANDLE; // or specify your render pass
	pipeline_info.subpass                      = 0;
	pipeline_info.basePipelineHandle           = VK_NULL_HANDLE;
	pipeline_info.pDynamicState                = &dynamic_state; // Enable dynamic state

	if (init.disp.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &data.graphicsPipeline) != VK_SUCCESS)
	{
		spdlog::error("Failed to create graphics pipeline!");
		return -1;
	}

	init.disp.destroyShaderModule(vert_module, nullptr);
	init.disp.destroyShaderModule(frag_module, nullptr);

	return 0;
}

int CreateCommandPool(Init& init, RenderData& data)
{
	spdlog::info("Creating command pool...");
	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex        = init.device.get_queue_index(vkb::QueueType::graphics).value(); // Assuming graphics queue family
	pool_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (init.disp.createCommandPool(&pool_info, nullptr, &data.commandPool) != VK_SUCCESS)
	{
		spdlog::error("Failed to create command pool!");
		return -1;
	}

	return 0;
}

int RecordCommandBuffers(Init& init, RenderData& data)
{
	spdlog::info("Recording command buffers...");
	data.commandBuffers.resize(init.swapchain.image_count);

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool                 = data.commandPool;
	alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount          = static_cast<uint32_t>(data.commandBuffers.size());

	if (init.disp.allocateCommandBuffers(&alloc_info, data.commandBuffers.data()) != VK_SUCCESS)
	{
		spdlog::error("Failed to allocate command buffers!");
		return -1;
	}

	return 0;
}

void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout)
{
	VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
	imageBarrier.pNext = nullptr;

	imageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
	imageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

	imageBarrier.oldLayout = currentLayout;
	imageBarrier.newLayout = newLayout;

	VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	imageBarrier.subresourceRange = { aspectMask, 0, 1, 0, 1 };
	imageBarrier.image            = image;

	VkDependencyInfo depInfo{};
	depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	depInfo.pNext = nullptr;

	depInfo.imageMemoryBarrierCount = 1;
	depInfo.pImageMemoryBarriers    = &imageBarrier;

	vkCmdPipelineBarrier2(cmd, &depInfo);
}

int Draw(Init& init, RenderData& data, VkCommandBuffer& cmd, int imageIndex)
{
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (init.disp.beginCommandBuffer(cmd, &begin_info) != VK_SUCCESS)
	{
		spdlog::error("Failed to begin recording command buffer!");
		return -1;
	}

	// Set dynamic viewport state
	VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)init.swapchain.extent.width,
		.height = (float)init.swapchain.extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkRect2D scissor = {
		.offset = { 0, 0 },
		.extent = init.swapchain.extent
	};

	init.disp.cmdSetViewport(cmd, 0, 1, &viewport); // Set dynamic viewport

	// Set dynamic scissor state
	init.disp.cmdSetScissor(cmd, 0, 1, &scissor); // Set dynamic scissor

	TransitionImage(cmd, data.swapchainImages[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	// Setup rendering info
	VkRenderingAttachmentInfo color_attachment_info = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = data.swapchainImageViews[imageIndex],
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = { .color = { { 0.05f, 0.05f, 0.05f, 1.0f } } },
	};

	VkRenderingInfo rendering_info          = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = { .extent = init.swapchain.extent },
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_info
	};

	init.disp.cmdBeginRendering(cmd, &rendering_info);

	init.disp.cmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, data.graphicsPipeline);

	// Perform drawing
	init.disp.cmdDraw(cmd, 3, 1, 0, 0);

	init.disp.cmdEndRendering(cmd);

	if (init.disp.endCommandBuffer(cmd) != VK_SUCCESS)
	{
		spdlog::error("Failed to record command buffer!");
		return -1;
	}

	return 0;
}

int RenderFrame(Init& init, RenderData& data)
{
	if (SlimeEngine::WindowSuspended(init.window))
	{
		return 0;
	}

	// Wait for the frame to be finished
	if (init.disp.waitForFences(1, &data.inFlightFences[data.currentFrame], VK_TRUE, UINT64_MAX) != VK_SUCCESS)
	{
		spdlog::error("Failed to wait for fence!");
		return -1;
	}

	uint32_t image_index;
	VkResult result = init.disp.acquireNextImageKHR(init.swapchain, UINT64_MAX, data.availableSemaphores[data.currentFrame], VK_NULL_HANDLE, &image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		if (CreateSwapchain(init, data) != 0)
			return -1;
		return 0;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		spdlog::error("Failed to acquire swapchain image!");
		return -1;
	}

	// Begin command buffer recording
	VkCommandBuffer cmd = data.commandBuffers[image_index];

	if (Draw(init, data, cmd, image_index) != 0)
		return -1;

	// Mark the image as now being in use by this frame
	data.imageInFlight[image_index] = data.inFlightFences[data.currentFrame];

	VkSubmitInfo submit_info = {};
	submit_info.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore wait_semaphores[]      = { data.availableSemaphores[data.currentFrame] };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.waitSemaphoreCount     = 1;
	submit_info.pWaitSemaphores        = wait_semaphores;
	submit_info.pWaitDstStageMask      = wait_stages;

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &cmd;

	VkSemaphore signal_semaphores[]  = { data.finishedSemaphore[data.currentFrame] };
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores    = signal_semaphores;

	init.disp.resetFences(1, &data.inFlightFences[data.currentFrame]);

	if (init.disp.queueSubmit(data.graphicsQueue, 1, &submit_info, data.inFlightFences[data.currentFrame]) != VK_SUCCESS)
	{
		spdlog::error("Failed to submit draw command buffer!");
		return -1;
	}

	VkPresentInfoKHR present_info   = {};
	present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores    = signal_semaphores;

	VkSwapchainKHR swapchains[] = { init.swapchain };
	present_info.swapchainCount = 1;
	present_info.pSwapchains    = swapchains;
	present_info.pImageIndices  = &image_index;

	result = init.disp.queuePresentKHR(data.presentQueue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || ShouldRecreateSwapchain(init.window))
	{
		if (CreateSwapchain(init, data) != 0)
			return -1;
	}
	else if (result != VK_SUCCESS)
	{
		spdlog::error("Failed to present swapchain image!");
		return -1;
	}

	data.currentFrame = (data.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return 0;
}

int InitSyncObjects(Init& init, RenderData& data)
{
	spdlog::info("Initializing synchronization objects...");
	// Create synchronization objects
	data.availableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	data.finishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
	data.inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	data.imageInFlight.resize(init.swapchain.image_count, VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info = {};
	fence_info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (init.disp.createSemaphore(&semaphore_info, nullptr, &data.availableSemaphores[i]) != VK_SUCCESS ||
		    init.disp.createSemaphore(&semaphore_info, nullptr, &data.finishedSemaphore[i]) != VK_SUCCESS ||
		    init.disp.createFence(&fence_info, nullptr, &data.inFlightFences[i]) != VK_SUCCESS)
		{
			spdlog::error("Failed to create synchronization objects!");
			return -1;
		}
	}

	return 0;
}

int Cleanup(Init& init, RenderData& data)
{
	spdlog::info("Cleaning up...");
	vkDeviceWaitIdle(init.device);

	// Destroy synchronization objects
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		init.disp.destroySemaphore(data.availableSemaphores[i], nullptr);
		init.disp.destroySemaphore(data.finishedSemaphore[i], nullptr);
		init.disp.destroyFence(data.inFlightFences[i], nullptr);
	}

	for (auto& image_view : data.swapchainImageViews)
	{
		init.disp.destroyImageView(image_view, nullptr);
	}

	init.disp.destroyPipeline(data.graphicsPipeline, nullptr);
	init.disp.destroyPipelineLayout(data.pipelineLayout, nullptr);
	init.disp.destroyCommandPool(data.commandPool, nullptr);

	vkb::destroy_swapchain(init.swapchain);
	vkb::destroy_surface(init.instance, init.surface);
	vkb::destroy_device(init.device);
	vkb::destroy_instance(init.instance);

	SlimeEngine::DestroyWindow(init.window);

	return 0;
}

}