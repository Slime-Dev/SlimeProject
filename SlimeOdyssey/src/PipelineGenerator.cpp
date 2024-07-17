#include "PipelineGenerator.h"

#include <fstream>
#include <stdexcept>

#include "VulkanContext.h"
#include "spdlog/spdlog.h"

PipelineGenerator::PipelineGenerator(VulkanContext& engine)
      : m_engine(engine), m_device(engine.GetDevice())
{
}

PipelineGenerator::~PipelineGenerator()
{
}

void PipelineGenerator::SetName(const std::string& name)
{
	m_pipelineContainer.name = name;
}

VkPipeline PipelineGenerator::GetPipeline() const
{
	return m_pipelineContainer.pipeline;
}

VkPipelineLayout PipelineGenerator::GetPipelineLayout() const
{
	return m_pipelineContainer.pipelineLayout;
}

const std::vector<VkDescriptorSet>& PipelineGenerator::GetDescriptorSets() const
{
	return m_pipelineContainer.descriptorSets;
}

PipelineContainer PipelineGenerator::GetPipelineContainer() const
{
	return m_pipelineContainer;
}

void PipelineGenerator::SetShaderModules(const ShaderModule& vertexShader, const ShaderModule& fragmentShader)
{
	m_vertexShader = vertexShader;
	m_fragmentShader = fragmentShader;
}

void PipelineGenerator::SetVertexInputState(const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions, const std::vector<VkVertexInputBindingDescription>& bindingDescription)
{
	m_attributeDescriptions = attributeDescriptions;
	m_vertexInputBindingDescriptions = bindingDescription;
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

void PipelineGenerator::SetDescriptorSets(const std::vector<VkDescriptorSet>& descriptorSets)
{
	m_pipelineContainer.descriptorSets = descriptorSets;
}

void PipelineGenerator::PrepareDescriptorSetLayouts(const ShaderManager::ShaderResources& resources)
{
	std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> setBindings;

	// Group bindings by set
	for (const auto& binding: resources.descriptorSetLayoutBindings)
	{
		setBindings[binding.set].push_back(binding.binding);
	}

	// Clear existing layouts
	for (auto layout: m_descriptorSetLayouts)
	{
		vkDestroyDescriptorSetLayout(m_device, layout, nullptr);
	}
	m_descriptorSetLayouts.clear();

	// Create a descriptor set layout for each set
	for (const auto& [set, bindings]: setBindings)
	{
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		VkDescriptorSetLayout setLayout;
		if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &setLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}

		m_descriptorSetLayouts.push_back(setLayout);

		spdlog::info("Created descriptor set layout for set {}", set);
		for (const auto& binding: bindings)
		{
			spdlog::info("  Binding {}: type {}, count {}, stage flags {}", binding.binding, static_cast<int>(binding.descriptorType), binding.descriptorCount, static_cast<int>(binding.stageFlags));
		}
	}

	// Store push constant ranges
	m_pushConstantRanges = resources.pushConstantRanges;
}

void PipelineGenerator::CreatePipelineLayout()
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = m_descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(m_pushConstantRanges.size());
	pipelineLayoutInfo.pPushConstantRanges = m_pushConstantRanges.data();

	spdlog::info("Creating pipeline layout");
	spdlog::info("Number of descriptor set layouts: {}", m_descriptorSetLayouts.size());
	for (size_t i = 0; i < m_descriptorSetLayouts.size(); ++i)
	{
		spdlog::info("Descriptor set layout {}: {}", i, (void*) m_descriptorSetLayouts[i]);
	}

	for (const auto& [stageFlags, offset, size]: m_pushConstantRanges)
	{
		spdlog::info("Push constant range: offset: {}, size: {}, stage flags: {}", offset, size, static_cast<int>(stageFlags));
	}

	if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineContainer.pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	m_engine.GetDebugUtils().SetObjectName(m_pipelineContainer.pipelineLayout, (m_pipelineContainer.name + " Pipeline Layout"));
	spdlog::info("Created pipeline layout: {}", (void*) m_pipelineContainer.pipelineLayout);
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

	VkPipelineShaderStageCreateInfo shaderStages[] = { m_vertexShaderStageInfo, m_fragmentShaderStageInfo };

	m_vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	m_vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(m_vertexInputBindingDescriptions.size());
	m_vertexInputInfo.pVertexBindingDescriptions = m_vertexInputBindingDescriptions.empty() ? nullptr : m_vertexInputBindingDescriptions.data();
	m_vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_attributeDescriptions.size());
	m_vertexInputInfo.pVertexAttributeDescriptions = m_attributeDescriptions.empty() ? nullptr : m_attributeDescriptions.data();

	m_inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	m_inputAssembly.primitiveRestartEnable = VK_FALSE;

	std::vector<VkDynamicState> dynamicStates =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT,
			VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT,
			VK_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	m_viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	m_viewportState.viewportCount = 1;
	m_viewportState.scissorCount = 1;

	m_rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_rasterizer.depthClampEnable = VK_FALSE;
	m_rasterizer.rasterizerDiscardEnable = VK_FALSE;
	m_rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	m_rasterizer.lineWidth = 1.0f; // Set later dynamically
	m_rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	m_rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	m_rasterizer.depthBiasEnable = VK_FALSE;

	m_multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	m_multisampling.sampleShadingEnable = VK_FALSE;
	m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	m_colorBlendAttachment.blendEnable = VK_TRUE;
	m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	m_colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	m_colorBlending.logicOpEnable = VK_FALSE;
	m_colorBlending.attachmentCount = 1;
	m_colorBlending.pAttachments = &m_colorBlendAttachment;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
	depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.front = {};
	depthStencilStateCreateInfo.back = {};
	depthStencilStateCreateInfo.minDepthBounds = 0.f;
	depthStencilStateCreateInfo.maxDepthBounds = 1.f;

	VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM; // swapchain image format
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

	VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {};
	pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;
	pipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;

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
	pipelineInfo.layout = m_pipelineContainer.pipelineLayout;
	pipelineInfo.renderPass = VK_NULL_HANDLE;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.pDepthStencilState = &depthStencilStateCreateInfo;
	pipelineInfo.pNext = &pipelineRenderingCreateInfo;

	if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipelineContainer.pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	m_engine.GetDebugUtils().SetObjectName(m_pipelineContainer.pipeline, (m_pipelineContainer.name + " Pipeline"));
}
