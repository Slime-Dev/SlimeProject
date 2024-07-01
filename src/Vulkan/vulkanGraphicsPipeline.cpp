#include "vulkanGraphicsPipeline.h"
#include "vulkanhelper.h"
#include "fileHelper.h"

#include <spdlog/spdlog.h>

namespace SlimeEngine
{
int CreateGraphicsPipeline(Init& init, RenderData& data, const ShaderConfig& shaderConfig, const PipelineConfig& config, const ColorBlendingConfig& blendingConfig)
{
spdlog::info("Creating graphics pipeline...");
	auto vert_code = SlimeEngine::ReadFile(shaderConfig.vertShaderPath);
	auto frag_code = SlimeEngine::ReadFile(shaderConfig.fragShaderPath);

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
	input_assembly.topology                               = config.topology;
	input_assembly.primitiveRestartEnable                 = config.primitiveRestartEnable;

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
	rasterizer.depthClampEnable                       = config.depthClampEnable;
	rasterizer.rasterizerDiscardEnable                = config.rasterizerDiscardEnable;
	rasterizer.polygonMode                            = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth                              = config.lineWidth;
	rasterizer.cullMode                               = config.cullMode;
	rasterizer.frontFace                              = config.frontFace;
	rasterizer.depthBiasEnable                        = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable                  = VK_FALSE;
	multisampling.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	color_blend_attachment.colorWriteMask                      = blendingConfig.colorWriteMask;
	color_blend_attachment.blendEnable                         = blendingConfig.blendEnable;

	VkPipelineColorBlendStateCreateInfo color_blending = {};
	color_blending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable                       = VK_FALSE;
	color_blending.logicOp                             = blendingConfig.logicOp;
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

	VkPipelineLayout& pipelineLayout = data.pipelineLayout.emplace_back();
	if (init.disp.createPipelineLayout(&pipeline_layout_info, nullptr, &pipelineLayout) != VK_SUCCESS)
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
	pipeline_info.layout                       = pipelineLayout;
	pipeline_info.renderPass                   = VK_NULL_HANDLE; // or specify your render pass
	pipeline_info.subpass                      = 0;
	pipeline_info.basePipelineHandle           = VK_NULL_HANDLE;
	pipeline_info.pDynamicState                = &dynamic_state; // Enable dynamic state

	VkPipeline& graphicsPipeline = data.graphicsPipeline.emplace_back();
	if (init.disp.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphicsPipeline) != VK_SUCCESS)
	{
		spdlog::error("Failed to create graphics pipeline!");
		return -1;
	}

	init.disp.destroyShaderModule(vert_module, nullptr);
	init.disp.destroyShaderModule(frag_module, nullptr);

	return 0;
}

}