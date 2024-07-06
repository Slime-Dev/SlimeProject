//
// Created by alexm on 3/07/24.
//

#pragma once

#pragma once

#include <string>
#include <vulkan/vulkan.h>
#include <vector>
#include "ShaderManager.h"

class PipelineGenerator {
public:
    PipelineGenerator(VkDevice device);

    ~PipelineGenerator();

    void SetShaderModules(const ShaderModule &vertexShader, const ShaderModule &fragmentShader);

    void SetVertexInputState(const std::vector<VkVertexInputAttributeDescription> &attributeDescriptions,
                             const std::vector<VkVertexInputBindingDescription> &bindingDescription);

    void SetDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout> &descriptorSetLayouts);

    void SetPushConstantRanges(const std::vector<VkPushConstantRange> &pushConstantRanges);

    void Generate();

    [[nodiscard]] VkPipeline getPipeline() const { return m_graphicsPipeline; }
    [[nodiscard]] VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }

private:
    VkDevice m_device;
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

    VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

    void CreatePipelineLayout();

    void CreatePipeline();

    void Cleanup();
};
