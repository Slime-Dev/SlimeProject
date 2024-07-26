#pragma once

#include <functional>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

#include "ShaderManager.h"

class VulkanContext;

struct PipelineConfig
{
	std::string name;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
};

class PipelineGenerator
{
public:
	explicit PipelineGenerator(VulkanContext& vulkanContext);
	~PipelineGenerator() = default;

	// Builder methods
	PipelineGenerator& SetName(const std::string& name);
	PipelineGenerator& SetShaderStages(const std::vector<VkPipelineShaderStageCreateInfo>& stages);
	PipelineGenerator& SetVertexInputState(const VkPipelineVertexInputStateCreateInfo& vertexInputInfo);
	PipelineGenerator& SetInputAssemblyState(const VkPipelineInputAssemblyStateCreateInfo& inputAssembly);
	PipelineGenerator& SetViewportState(const VkPipelineViewportStateCreateInfo& viewportState);
	PipelineGenerator& SetRasterizationState(const VkPipelineRasterizationStateCreateInfo& rasterizer);
	PipelineGenerator& SetMultisampleState(const VkPipelineMultisampleStateCreateInfo& multisampling);
	PipelineGenerator& SetDepthStencilState(const VkPipelineDepthStencilStateCreateInfo& depthStencil);
	PipelineGenerator& SetColorBlendState(const VkPipelineColorBlendStateCreateInfo& colorBlending);
	PipelineGenerator& SetDynamicState(const VkPipelineDynamicStateCreateInfo& dynamicState);
	PipelineGenerator& SetLayout(VkPipelineLayout layout);
	PipelineGenerator& SetRenderPass(VkRenderPass renderPass, uint32_t subpass);
	PipelineGenerator& SetBasePipeline(VkPipeline basePipeline, int32_t basePipelineIndex);
	PipelineGenerator& SetRenderingInfo(VkPipelineRenderingCreateInfo renderingInfo);

	// Convenience methods for common configurations
	PipelineGenerator& SetDefaultVertexInput();
	PipelineGenerator& SetDefaultInputAssembly();
	PipelineGenerator& SetDefaultViewportState();
	PipelineGenerator& SetDefaultRasterizationState();
	PipelineGenerator& SetDefaultMultisampleState();
	PipelineGenerator& SetDefaultDepthStencilState();
	PipelineGenerator& SetDefaultColorBlendState();
	PipelineGenerator& SetDefaultDynamicState();

	// Advanced configuration
	PipelineGenerator& AddDynamicState(VkDynamicState state);
	PipelineGenerator& SetPolygonMode(VkPolygonMode mode);
	PipelineGenerator& SetCullMode(VkCullModeFlags mode);
	PipelineGenerator& SetDepthTestEnable(bool enable);
	PipelineGenerator& SetDepthWriteEnable(bool enable);
	PipelineGenerator& SetDepthCompareOp(VkCompareOp op);
	PipelineGenerator& SetBlendEnable(bool enable);
	PipelineGenerator& SetColorBlendOp(VkBlendOp op);
	PipelineGenerator& SetAlphaBlendOp(VkBlendOp op);

	// Descriptor set and push constant configuration
	PipelineGenerator& SetDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout>& layouts);
	PipelineGenerator& SetPushConstantRanges(const std::vector<VkPushConstantRange>& ranges);

	// Build methods
	PipelineConfig Build();
	void Reset();

private:
	VulkanContext& m_vulkanContext;
	const vkb::DispatchTable& m_disp;

	// Pipeline state
	std::string m_name;
	std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
	std::optional<VkPipelineVertexInputStateCreateInfo> m_vertexInputState;
	std::optional<VkPipelineInputAssemblyStateCreateInfo> m_inputAssemblyState;
	std::optional<VkPipelineViewportStateCreateInfo> m_viewportState;
	std::optional<VkPipelineRasterizationStateCreateInfo> m_rasterizationState;
	std::optional<VkPipelineMultisampleStateCreateInfo> m_multisampleState;
	std::optional<VkPipelineDepthStencilStateCreateInfo> m_depthStencilState;
	std::optional<VkPipelineColorBlendStateCreateInfo> m_colorBlendState;
	std::optional<VkPipelineDynamicStateCreateInfo> m_dynamicState;
	std::optional<VkPipelineRenderingCreateInfo> m_renderingInfo;
	VkPipelineLayout m_layout = VK_NULL_HANDLE;
	VkRenderPass m_renderPass = VK_NULL_HANDLE;
	uint32_t m_subpass = 0;
	VkPipeline m_basePipeline = VK_NULL_HANDLE;
	int32_t m_basePipelineIndex = -1;

	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	std::vector<VkPushConstantRange> m_pushConstantRanges;

	// Helper methods
	void CreatePipelineLayout();
	void CreatePipeline();
};
