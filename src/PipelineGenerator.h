//
// Created by alexm on 3/07/24.
//

#pragma once

#include <string>
#include <vulkan/vulkan.h>
#include <vector>

namespace spirv_cross
{
struct SPIRType;
}

class PipelineGenerator
{
public:
	PipelineGenerator(VkDevice device, const std::vector<uint32_t>& vertexShaderCode, const std::vector<uint32_t>& fragmentShaderCode);
	~PipelineGenerator();

	void setShaderModules(VkShaderModule vertexShaderModule, VkShaderModule fragmentShaderModule);
	void generate();
	[[nodiscard]] VkPipeline getPipeline() const { return m_graphicsPipeline; }
	[[nodiscard]] VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }

private:
	VkDevice m_device;
	const std::vector<uint32_t>& m_vertexShaderCode;
	const std::vector<uint32_t>& m_fragmentShaderCode;
	VkShaderModule m_vertexShaderModule = VK_NULL_HANDLE;
	VkShaderModule m_fragmentShaderModule = VK_NULL_HANDLE;
	std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions;

	VkDescriptorSetLayout m_descriptorSetLayout; // TODO: Store this in a resource manager or something !it is leaking atm

	VkVertexInputBindingDescription m_vertexInputBindingDescription{};
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

	VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

	void extractShaderInfo();
	VkFormat getVkFormat(const spirv_cross::SPIRType& type);
	uint32_t getSizeOfFormat(VkFormat format);
	void createPipeline();
	void cleanup();
};