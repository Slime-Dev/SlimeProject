#include "PipelineGenerator.h"
#include <fstream>
#include <stdexcept>
#include "spirv_glsl.hpp"

PipelineGenerator::PipelineGenerator(VkDevice device)
	: m_device(device)
{
}

PipelineGenerator::~PipelineGenerator()
{
	Cleanup();
}

void PipelineGenerator::SetShaderModules(const ShaderModule& vertexShader, const ShaderModule& fragmentShader)
{
	m_vertexShader   = vertexShader;
	m_fragmentShader = fragmentShader;
}

void PipelineGenerator::SetVertexInputState(const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions,
	const VkVertexInputBindingDescription& bindingDescription)
{
	m_attributeDescriptions         = attributeDescriptions;
	m_vertexInputBindingDescription = bindingDescription;
}

void PipelineGenerator::SetDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
{
	m_descriptorSetLayouts = descriptorSetLayouts;
}

void PipelineGenerator::SetPushConstantRanges(const std::vector<VkPushConstantRange>& pushConstantRanges)
{
	m_pushConstantRanges = pushConstantRanges;
}

void PipelineGenerator::Generate()
{
	CreatePipelineLayout();
	CreatePipeline();
}

void PipelineGenerator::CreatePipelineLayout()
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = m_descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(m_pushConstantRanges.size());
	pipelineLayoutInfo.pPushConstantRanges = m_pushConstantRanges.data();

	if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
}
void PipelineGenerator::CreatePipeline()
{
    m_vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    m_vertexShaderStageInfo.module = m_vertexShader.handle;
    m_vertexShaderStageInfo.pName = "main";

    m_fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    m_fragmentShaderStageInfo.module = m_fragmentShader.handle;
    m_fragmentShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {m_vertexShaderStageInfo, m_fragmentShaderStageInfo};

    m_vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    m_vertexInputInfo.vertexBindingDescriptionCount = m_attributeDescriptions.empty() ? 0 : 1;
    m_vertexInputInfo.pVertexBindingDescriptions = &m_vertexInputBindingDescription;
    m_vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_attributeDescriptions.size());
    m_vertexInputInfo.pVertexAttributeDescriptions = m_attributeDescriptions.data();

    m_inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    m_viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    m_viewportState.viewportCount = 1;
    m_viewportState.scissorCount = 1;

    m_rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_rasterizer.depthClampEnable = VK_FALSE;
    m_rasterizer.rasterizerDiscardEnable = VK_FALSE;
    m_rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    m_rasterizer.lineWidth = 1.0f;
    m_rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    m_rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    m_rasterizer.depthBiasEnable = VK_FALSE;

    m_multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_multisampling.sampleShadingEnable = VK_FALSE;
    m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    m_colorBlendAttachment.blendEnable = VK_FALSE;

    m_colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_colorBlending.logicOpEnable = VK_FALSE;
    m_colorBlending.attachmentCount = 1;
    m_colorBlending.pAttachments = &m_colorBlendAttachment;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &m_vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &m_inputAssembly;
    pipelineInfo.pViewportState = &m_viewportState;
    pipelineInfo.pRasterizationState = &m_rasterizer;
    pipelineInfo.pMultisampleState = &m_multisampling;
    pipelineInfo.pColorBlendState = &m_colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
}

void PipelineGenerator::Cleanup()
{
    if (m_graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
        m_graphicsPipeline = VK_NULL_HANDLE;
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
}