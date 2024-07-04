#include "PipelineGenerator.h"
#include <fstream>
#include <stdexcept>
#include "spirv_glsl.hpp"

PipelineGenerator::PipelineGenerator(VkDevice device, const std::vector<uint32_t>& vertexShaderCode, const std::vector<uint32_t>& fragmentShaderCode)
	: m_device(device), m_vertexShaderCode(vertexShaderCode), m_fragmentShaderCode(fragmentShaderCode)
{
}

PipelineGenerator::~PipelineGenerator()
{
	cleanup();
}

void PipelineGenerator::setShaderModules(VkShaderModule vertexShaderModule, VkShaderModule fragmentShaderModule)
{
	m_vertexShaderModule   = vertexShaderModule;
	m_fragmentShaderModule = fragmentShaderModule;
}

void PipelineGenerator::generate()
{
	if (m_vertexShaderModule == VK_NULL_HANDLE || m_fragmentShaderModule == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Shader modules must be set before generating pipeline");
	}

	extractShaderInfo();
	createPipeline();
}

void PipelineGenerator::extractShaderInfo()
{
	spirv_cross::CompilerGLSL vertexCompiler(m_vertexShaderCode);
	spirv_cross::ShaderResources vertResources = vertexCompiler.get_shader_resources();

	// Extract vertex input attributes
	uint32_t offset = 0;
	for (const auto& resource : vertResources.stage_inputs)
	{
		uint32_t location = vertexCompiler.get_decoration(resource.id, spv::DecorationLocation);

		VkVertexInputAttributeDescription attributeDescription{};
		attributeDescription.location = location;
		attributeDescription.binding  = 0;
		attributeDescription.format   = getVkFormat(vertexCompiler.get_type(resource.base_type_id));
		attributeDescription.offset   = offset;

		m_attributeDescriptions.push_back(attributeDescription);
		offset += getSizeOfFormat(attributeDescription.format);
	}

	// Set up vertex input binding
	m_vertexInputBindingDescription.binding   = 0;
	m_vertexInputBindingDescription.stride    = offset;
	m_vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	m_vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	m_vertexInputInfo.vertexBindingDescriptionCount   = m_attributeDescriptions.empty() ? 0 : 1;
	m_vertexInputInfo.pVertexBindingDescriptions      = &m_vertexInputBindingDescription;
	m_vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_attributeDescriptions.size());
	m_vertexInputInfo.pVertexAttributeDescriptions    = m_attributeDescriptions.data();

	// Extract push constants, uniform buffers, etc.
	std::vector<VkPushConstantRange> pushConstantRanges;
	for (const auto& resource : vertResources.push_constant_buffers)
	{
		const auto& bufferType = vertexCompiler.get_type(resource.base_type_id);
		VkPushConstantRange range{};
		range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		range.offset     = 0;
		range.size       = vertexCompiler.get_declared_struct_size(bufferType);
		pushConstantRanges.push_back(range);
	}

	// Set up descriptor set layouts (for uniform buffers, etc.)
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
	for (const auto& resource : vertResources.uniform_buffers)
	{
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding         = vertexCompiler.get_decoration(resource.id, spv::DecorationBinding);
		layoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBinding.descriptorCount = 1;
		layoutBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
		layoutBindings.push_back(layoutBinding);
	}

	// Create descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	layoutInfo.pBindings    = layoutBindings.data();

	if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	// Set up pipeline layout
	m_pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	m_pipelineLayoutInfo.setLayoutCount         = 1;
	m_pipelineLayoutInfo.pSetLayouts            = &m_descriptorSetLayout;
	m_pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
	m_pipelineLayoutInfo.pPushConstantRanges    = pushConstantRanges.data();

	if (vkCreatePipelineLayout(m_device, &m_pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}
}

void PipelineGenerator::createPipeline()
{
	m_vertexShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	m_vertexShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
	m_vertexShaderStageInfo.module = m_vertexShaderModule;
	m_vertexShaderStageInfo.pName  = "main";

	m_fragmentShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	m_fragmentShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	m_fragmentShaderStageInfo.module = m_fragmentShaderModule;
	m_fragmentShaderStageInfo.pName  = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { m_vertexShaderStageInfo, m_fragmentShaderStageInfo };

	m_inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	m_inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates    = dynamicStates;

	m_viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	m_viewportState.viewportCount = 1;
	m_viewportState.scissorCount  = 1;

	m_rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_rasterizer.depthClampEnable        = VK_FALSE;
	m_rasterizer.rasterizerDiscardEnable = VK_FALSE;
	m_rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
	m_rasterizer.lineWidth               = 1.0f;
	m_rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
	m_rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
	m_rasterizer.depthBiasEnable         = VK_FALSE;

	m_multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	m_multisampling.sampleShadingEnable  = VK_FALSE;
	m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_colorBlendAttachment.blendEnable    = VK_FALSE;

	m_colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	m_colorBlending.logicOpEnable   = VK_FALSE;
	m_colorBlending.attachmentCount = 1;
	m_colorBlending.pAttachments    = &m_colorBlendAttachment;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount          = 2;
	pipelineInfo.pStages             = shaderStages;
	pipelineInfo.pVertexInputState   = &m_vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &m_inputAssembly;
	pipelineInfo.pViewportState      = &m_viewportState;
	pipelineInfo.pRasterizationState = &m_rasterizer;
	pipelineInfo.pMultisampleState   = &m_multisampling;
	pipelineInfo.pColorBlendState    = &m_colorBlending;
	pipelineInfo.pDynamicState       = &dynamicState;
	pipelineInfo.layout              = m_pipelineLayout;
	pipelineInfo.renderPass          = VK_NULL_HANDLE;
	pipelineInfo.subpass             = 0;
	pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}
}

VkFormat PipelineGenerator::getVkFormat(const spirv_cross::SPIRType& type)
{
	if (type.basetype == spirv_cross::SPIRType::Float)
	{
		if (type.vecsize == 1)
			return VK_FORMAT_R32_SFLOAT;
		if (type.vecsize == 2)
			return VK_FORMAT_R32G32_SFLOAT;
		if (type.vecsize == 3)
			return VK_FORMAT_R32G32B32_SFLOAT;
		if (type.vecsize == 4)
			return VK_FORMAT_R32G32B32A32_SFLOAT;
	}
	throw std::runtime_error("Unsupported format in shader");
}

uint32_t PipelineGenerator::getSizeOfFormat(VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_R32_SFLOAT:
		return 4;
	case VK_FORMAT_R32G32_SFLOAT:
		return 8;
	case VK_FORMAT_R32G32B32_SFLOAT:
		return 12;
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		return 16;
	default:
		throw std::runtime_error("Unsupported format");
	}
}

void PipelineGenerator::cleanup()
{
	if (m_descriptorSetLayout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
		m_descriptorSetLayout = VK_NULL_HANDLE;
	}

	if (m_graphicsPipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
		m_graphicsPipeline = VK_NULL_HANDLE;
	}
	if (m_pipelineLayout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
		m_pipelineLayout = VK_NULL_HANDLE;
	}
}