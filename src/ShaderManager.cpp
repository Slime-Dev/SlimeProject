//
// Created by alexm on 3/07/24.
//
#include "ShaderManager.h"
#include <fstream>
#include <stdexcept>
#include "spirv_cross.hpp"
#include "spirv_glsl.hpp"

ShaderManager::ShaderManager(VkDevice device) : m_device(device)
{
}

ShaderManager::~ShaderManager()
{
	CleanupDescriptorSetLayouts();
	CleanupShaderModules();
}

ShaderModule ShaderManager::LoadShader(const std::string& path, VkShaderStageFlagBits stage)
{
	auto it = m_shaderModules.find(path);
	if (it != m_shaderModules.end())
	{
		return it->second;
	}

	std::vector<uint32_t> code  = ReadFile(path);
	VkShaderModule shaderModule = CreateShaderModule(code);
	ShaderModule module(shaderModule, std::move(code), stage);
	m_shaderModules[path] = module;

	return module;
}

ShaderManager::ShaderResources ShaderManager::ParseShader(const ShaderModule& shaderModule)
{
	ShaderResources resources;
	spirv_cross::CompilerGLSL compiler(shaderModule.spirvCode);
	spirv_cross::ShaderResources shaderResources = compiler.get_shader_resources();

	// Parse vertex input attributes
	if (shaderModule.stage == VK_SHADER_STAGE_VERTEX_BIT)
	{
		uint32_t location = 0;
		for (const auto& resource : shaderResources.stage_inputs)
		{
			const auto& type = compiler.get_type(resource.base_type_id);
			VkFormat format  = GetVkFormat(type);

			VkVertexInputAttributeDescription attributeDescription{};
			attributeDescription.location = location++;
			attributeDescription.binding  = 0;
			attributeDescription.format   = format;
			attributeDescription.offset   = 0; // Will be calculated later

			resources.attributeDescriptions.push_back(attributeDescription);
		}

		// Set up vertex input binding
		resources.bindingDescription.binding   = 0;
		resources.bindingDescription.stride    = 0; // Will be calculated later
		resources.bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	}

	// Parse uniform buffers
	for (const auto& resource : shaderResources.uniform_buffers)
	{
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding         = compiler.get_decoration(resource.id, spv::DecorationBinding);
		layoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBinding.descriptorCount = 1;
		layoutBinding.stageFlags      = shaderModule.stage;

		resources.descriptorSetLayoutBindings.push_back(layoutBinding);
	}

	// Parse push constants
	for (const auto& resource : shaderResources.push_constant_buffers)
	{
		const auto& bufferType = compiler.get_type(resource.base_type_id);
		VkPushConstantRange range{};
		range.stageFlags = shaderModule.stage;
		range.offset     = 0;
		range.size       = compiler.get_declared_struct_size(bufferType);

		resources.pushConstantRanges.push_back(range);
	}

	return resources;
}

std::string HashBindings(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
	std::string hash = "descriptor_set_layout";
	for (const auto& binding : bindings)
	{
		hash += std::to_string(binding.binding) + std::to_string(binding.descriptorType) + std::to_string(binding.descriptorCount) + std::to_string(binding.stageFlags);
	}
	return hash;
}

VkDescriptorSetLayout ShaderManager::CreateDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
	std::string hash = HashBindings(bindings);

	if (m_descriptorSetLayouts.find(hash) != m_descriptorSetLayouts.end())
	{
		return m_descriptorSetLayouts[hash];
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings    = bindings.data();

	VkDescriptorSetLayout descriptorSetLayout;
	if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	m_descriptorSetLayouts[hash] = descriptorSetLayout;

	return descriptorSetLayout;
}

void ShaderManager::CleanupShaderModules()
{
	for (const auto& [path, shaderModule] : m_shaderModules)
	{
		vkDestroyShaderModule(m_device, shaderModule.handle, nullptr);
	}
	m_shaderModules.clear();
}

void ShaderManager::CleanupDescriptorSetLayouts()
{
	for (const auto& [hash, descriptorSetLayout] : m_descriptorSetLayouts)
	{
		vkDestroyDescriptorSetLayout(m_device, descriptorSetLayout, nullptr);
	}
	m_descriptorSetLayouts.clear();
}

std::vector<uint32_t> ShaderManager::ReadFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file: " + filename);
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
	file.close();

	return buffer;
}

VkShaderModule ShaderManager::CreateShaderModule(const std::vector<uint32_t>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size() * sizeof(uint32_t);
	createInfo.pCode    = code.data();

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

VkFormat ShaderManager::GetVkFormat(const spirv_cross::SPIRType& type)
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