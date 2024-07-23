//
// Created by alexm on 3/07/24.
//

#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "ShaderManager.h"

class VulkanContext;
class ModelManager;
class DescriptorManager;
namespace vkb { struct DispatchTable; }

struct PipelineContainer
{
	std::string name;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
};

class PipelineGenerator
{
public:
	explicit PipelineGenerator(VulkanContext& vulkanContext);

	~PipelineGenerator();

	void SetName(const std::string& name);

	void SetShaderModules(const ShaderModule& vertexShader, const ShaderModule& fragmentShader);
	void SetVertexInputState(const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions, const std::vector<VkVertexInputBindingDescription>& bindingDescription);
	void SetDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
	void SetPushConstantRanges(const std::vector<VkPushConstantRange>& pushConstantRanges);
	void SetDescriptorSets(const std::vector<VkDescriptorSet>& descriptorSets);
	void SetDepthTestEnabled(bool enabled);
	void SetCullMode(VkCullModeFlags mode);
	void SetPolygonMode(VkPolygonMode polygonMode);

	void Generate();

	void PrepareDescriptorSetLayouts(const ShaderManager::ShaderResources& resources);

	[[nodiscard]] VkPipeline GetPipeline() const;
	[[nodiscard]] VkPipelineLayout GetPipelineLayout() const;
	[[nodiscard]] const std::vector<VkDescriptorSet>& GetDescriptorSets() const;
	[[nodiscard]] PipelineContainer GetPipelineContainer() const;

private:
	const vkb::DispatchTable& m_disp;
	VulkanContext& m_vulkanContext;
	ShaderModule m_vertexShader;
	ShaderModule m_fragmentShader;

	std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions;
	std::vector<VkVertexInputBindingDescription> m_vertexInputBindingDescriptions;
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	std::vector<VkPushConstantRange> m_pushConstantRanges;

	VkPipelineShaderStageCreateInfo m_vertexShaderStageInfo{};
	VkPipelineShaderStageCreateInfo m_fragmentShaderStageInfo{};
	VkPipelineVertexInputStateCreateInfo m_vertexInputInfo{};
	VkPipelineInputAssemblyStateCreateInfo m_inputAssembly{};
	VkPipelineViewportStateCreateInfo m_viewportState{};
	VkPipelineRasterizationStateCreateInfo m_rasterizer{};
	VkPipelineMultisampleStateCreateInfo m_multisampling{};
	VkPipelineColorBlendAttachmentState m_colorBlendAttachment{};
	VkPipelineColorBlendStateCreateInfo m_colorBlending{};
	VkPipelineLayoutCreateInfo m_pipelineLayoutInfo{};

	VkPolygonMode m_vkPolygonMode = VK_POLYGON_MODE_FILL;

	bool m_dephtestEnabled = true;
	VkCullModeFlags m_cullMode = VK_CULL_MODE_BACK_BIT;

	PipelineContainer m_pipelineContainer;

	void CreatePipelineLayout();
	void CreatePipeline();

};
