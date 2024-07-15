//
// Created by alexm on 3/07/24.
//

#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

#include "spirv_common.hpp"

struct ShaderModule
{
	VkShaderModule handle;
	std::vector<uint32_t> spirvCode;
	VkShaderStageFlagBits stage;

	ShaderModule(VkShaderModule _handle, std::vector<uint32_t> _spirvCode, VkShaderStageFlagBits _stage)
	      : handle(_handle), spirvCode(std::move(_spirvCode)), stage(_stage)
	{
	}

	ShaderModule() = default;
};

class ShaderManager
{
public:
	struct ShaderResources
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;

		struct DescriptorSetLayoutBinding
		{
			uint32_t set;
			VkDescriptorSetLayoutBinding binding;
		};

		std::vector<DescriptorSetLayoutBinding> descriptorSetLayoutBindings;
		std::vector<VkPushConstantRange> pushConstantRanges;
	};

	ShaderManager() = default;
	~ShaderManager() = default;

	void CleanUp(VkDevice device);

	ShaderModule LoadShader(VkDevice device, const std::string& path, VkShaderStageFlagBits stage);
	ShaderResources ParseShader(const ShaderModule& shaderModule);
	ShaderResources CombineResources(const std::vector<ShaderModule>& shaderModules);

	std::vector<VkDescriptorSetLayout> CreateDescriptorSetLayouts(VkDevice device, const ShaderResources& resources);

	void CleanupShaderModules(VkDevice device);
	void CleanupDescriptorSetLayouts(VkDevice device);

private:
	std::unordered_map<std::string, ShaderModule> m_shaderModules;
	std::unordered_map<std::string, VkDescriptorSetLayout> m_descriptorSetLayouts;
	std::map<uint32_t, uint32_t> bindingOffsets;

	std::vector<uint32_t> ReadFile(const std::string& filename);
	VkShaderModule CreateShaderModule(VkDevice device, const std::vector<uint32_t>& code);
	VkFormat GetVkFormat(const spirv_cross::SPIRType& type);
	uint32_t GetFormatSize(VkFormat format);
};
