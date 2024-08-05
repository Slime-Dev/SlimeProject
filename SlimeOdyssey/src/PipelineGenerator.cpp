#include "PipelineGenerator.h"

#include <VkBootstrapDispatch.h>

#include "ShaderManager.h"
#include "spdlog/spdlog.h"
#include "VulkanUtil.h"

PipelineGenerator::PipelineGenerator()
{
}

PipelineGenerator& PipelineGenerator::SetName(const std::string& name)
{
	m_name = name;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetShaderStages(const std::vector<VkPipelineShaderStageCreateInfo>& stages)
{
	m_shaderStages = stages;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetVertexInputState(const VkPipelineVertexInputStateCreateInfo& vertexInputInfo)
{
	m_vertexInputState = vertexInputInfo;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetInputAssemblyState(const VkPipelineInputAssemblyStateCreateInfo& inputAssembly)
{
	m_inputAssemblyState = inputAssembly;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetViewportState(const VkPipelineViewportStateCreateInfo& viewportState)
{
	m_viewportState = viewportState;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetRasterizationState(const VkPipelineRasterizationStateCreateInfo& rasterizer)
{
	m_rasterizationState = rasterizer;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetMultisampleState(const VkPipelineMultisampleStateCreateInfo& multisampling)
{
	m_multisampleState = multisampling;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetDepthStencilState(const VkPipelineDepthStencilStateCreateInfo& depthStencil)
{
	m_depthStencilState = depthStencil;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetColorBlendState(const VkPipelineColorBlendStateCreateInfo& colorBlending)
{
	m_colorBlendState = colorBlending;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetDynamicState(const VkPipelineDynamicStateCreateInfo& dynamicState)
{
	m_dynamicState = dynamicState;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetLayout(VkPipelineLayout layout)
{
	m_layout = layout;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetRenderPass(VkRenderPass renderPass, uint32_t subpass)
{
	m_renderPass = renderPass;
	m_subpass = subpass;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetBasePipeline(VkPipeline basePipeline, int32_t basePipelineIndex)
{
	m_basePipeline = basePipeline;
	m_basePipelineIndex = basePipelineIndex;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetRenderingInfo(VkPipelineRenderingCreateInfo renderingInfo)
{
	m_renderingInfo = renderingInfo;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetDefaultVertexInput()
{
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	m_vertexInputState = vertexInputInfo;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetDefaultInputAssembly()
{
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;
	m_inputAssemblyState = inputAssembly;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetDefaultViewportState()
{
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	m_viewportState = viewportState;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetDefaultRasterizationState()
{
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	m_rasterizationState = rasterizer;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetDefaultMultisampleState()
{
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	m_multisampleState = multisampling;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetDefaultDepthStencilState()
{
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;
	m_depthStencilState = depthStencil;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetDefaultColorBlendState()
{
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	m_colorBlendState = colorBlending;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetDefaultDynamicState()
{
	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();
	m_dynamicState = dynamicState;
	return *this;
}

PipelineGenerator& PipelineGenerator::AddDynamicState(VkDynamicState state)
{
	if (!m_dynamicState)
	{
		SetDefaultDynamicState();
	}
	auto& dynamicStates = const_cast<std::vector<VkDynamicState>&>(*reinterpret_cast<const std::vector<VkDynamicState>*>(m_dynamicState->pDynamicStates));
	dynamicStates.push_back(state);
	m_dynamicState->dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	m_dynamicState->pDynamicStates = dynamicStates.data();
	return *this;
}

PipelineGenerator& PipelineGenerator::SetPolygonMode(VkPolygonMode mode)
{
	if (!m_rasterizationState)
	{
		SetDefaultRasterizationState();
	}
	m_rasterizationState->polygonMode = mode;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetCullMode(VkCullModeFlags mode)
{
	if (!m_rasterizationState)
	{
		SetDefaultRasterizationState();
	}
	m_rasterizationState->cullMode = mode;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetDepthTestEnable(bool enable)
{
	if (!m_depthStencilState)
	{
		SetDefaultDepthStencilState();
	}
	m_depthStencilState->depthTestEnable = enable ? VK_TRUE : VK_FALSE;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetDepthWriteEnable(bool enable)
{
	if (!m_depthStencilState)
	{
		SetDefaultDepthStencilState();
	}
	m_depthStencilState->depthWriteEnable = enable ? VK_TRUE : VK_FALSE;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetDepthCompareOp(VkCompareOp op)
{
	if (!m_depthStencilState)
	{
		SetDefaultDepthStencilState();
	}
	m_depthStencilState->depthCompareOp = op;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetBlendEnable(bool enable)
{
	if (!m_colorBlendState)
	{
		SetDefaultColorBlendState();
	}
	auto& attachment = const_cast<VkPipelineColorBlendAttachmentState&>(*m_colorBlendState->pAttachments);
	attachment.blendEnable = enable ? VK_TRUE : VK_FALSE;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetColorBlendOp(VkBlendOp op)
{
	if (!m_colorBlendState)
	{
		SetDefaultColorBlendState();
	}
	auto& attachment = const_cast<VkPipelineColorBlendAttachmentState&>(*m_colorBlendState->pAttachments);
	attachment.colorBlendOp = op;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetAlphaBlendOp(VkBlendOp op)
{
	if (!m_colorBlendState)
	{
		SetDefaultColorBlendState();
	}
	auto& attachment = const_cast<VkPipelineColorBlendAttachmentState&>(*m_colorBlendState->pAttachments);
	attachment.alphaBlendOp = op;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout>& layouts)
{
	m_descriptorSetLayouts = layouts;
	return *this;
}

PipelineGenerator& PipelineGenerator::SetPushConstantRanges(const std::vector<VkPushConstantRange>& ranges)
{
	m_pushConstantRanges = ranges;
	return *this;
}

void PipelineGenerator::CreatePipelineLayout(vkb::DispatchTable& disp, VulkanDebugUtils& debugUtils)
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = m_descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(m_pushConstantRanges.size());
	pipelineLayoutInfo.pPushConstantRanges = m_pushConstantRanges.data();

	VK_CHECK(disp.createPipelineLayout(&pipelineLayoutInfo, nullptr, &m_layout));
	debugUtils.SetObjectName(m_layout, m_name + " Pipeline Layout");
}

void PipelineGenerator::CreatePipeline(vkb::DispatchTable& disp, VulkanDebugUtils& debugUtils)
{
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(m_shaderStages.size());
	pipelineInfo.pStages = m_shaderStages.data();
	pipelineInfo.pVertexInputState = m_vertexInputState ? &*m_vertexInputState : nullptr;
	pipelineInfo.pInputAssemblyState = m_inputAssemblyState ? &*m_inputAssemblyState : nullptr;
	pipelineInfo.pViewportState = m_viewportState ? &*m_viewportState : nullptr;
	pipelineInfo.pRasterizationState = m_rasterizationState ? &*m_rasterizationState : nullptr;
	pipelineInfo.pMultisampleState = m_multisampleState ? &*m_multisampleState : nullptr;
	pipelineInfo.pDepthStencilState = m_depthStencilState ? &*m_depthStencilState : nullptr;
	pipelineInfo.pColorBlendState = m_colorBlendState ? &*m_colorBlendState : nullptr;
	pipelineInfo.pDynamicState = m_dynamicState ? &*m_dynamicState : nullptr;
	pipelineInfo.layout = m_layout;
	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.subpass = m_subpass;
	pipelineInfo.basePipelineHandle = m_basePipeline;
	pipelineInfo.basePipelineIndex = m_basePipelineIndex;
	pipelineInfo.pNext = &m_renderingInfo;

	VK_CHECK(disp.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_basePipeline));

	debugUtils.SetObjectName(m_basePipeline, (m_name + " Pipeline"));
}

PipelineConfig PipelineGenerator::Build(vkb::DispatchTable& disp, VulkanDebugUtils& debugUtils)
{
	// Create pipeline layout
	CreatePipelineLayout(disp, debugUtils);

	// Create the graphics pipeline
	CreatePipeline(disp, debugUtils);

	// Create and return the PipelineConfig
	PipelineConfig config;
	config.name = m_name;
	config.pipelineLayout = m_layout;
	config.pipeline = m_basePipeline;
	config.descriptorSetLayouts = m_descriptorSetLayouts;

	return config;
}

void PipelineGenerator::Reset()
{
	m_name.clear();
	m_shaderStages.clear();
	m_vertexInputState.reset();
	m_inputAssemblyState.reset();
	m_viewportState.reset();
	m_rasterizationState.reset();
	m_multisampleState.reset();
	m_depthStencilState.reset();
	m_colorBlendState.reset();
	m_dynamicState.reset();
	m_layout = VK_NULL_HANDLE;
	m_renderPass = VK_NULL_HANDLE;
	m_subpass = 0;
	m_basePipeline = VK_NULL_HANDLE;
	m_basePipelineIndex = -1;
	m_descriptorSetLayouts.clear();
	m_pushConstantRanges.clear();
}
