//
// Created by alexm on 3/07/24.
//
#include "ShaderManager.h"

#include <fstream>
#include <stdexcept>

#include "spdlog/spdlog.h"
#include "spirv_cross.hpp"
#include "spirv_glsl.hpp"

ShaderManager::ShaderManager(VkDevice device)
      : m_device(device)
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

	std::vector<uint32_t> code = ReadFile(path);
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
		std::unordered_map<uint32_t, uint32_t> bindingOffsets;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		attributeDescriptions.reserve(shaderResources.stage_inputs.size());

		// First pass: collect all attributes
		for (const auto& resource: shaderResources.stage_inputs)
		{
			const auto& type = compiler.get_type(resource.base_type_id);
			VkFormat format = GetVkFormat(type);

			uint32_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

			attributeDescriptions.push_back({
			        .location = location,
			        .binding = binding,
			        .format = format,
			        .offset = 0 // We'll set this correctly in the second pass
			});
		}

		// Sort attribute descriptions
		std::sort(attributeDescriptions.begin(), attributeDescriptions.end(), [](const auto& a, const auto& b) { return std::tie(a.binding, a.location) < std::tie(b.binding, b.location); });

		// Second pass: calculate correct offsets
		for (auto& attr: attributeDescriptions)
		{
			attr.offset = bindingOffsets[attr.binding];
			bindingOffsets[attr.binding] += GetFormatSize(attr.format);
		}

		// Set up vertex input bindings
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		bindingDescriptions.reserve(bindingOffsets.size());

		for (const auto& [binding, offset]: bindingOffsets)
		{
			bindingDescriptions.push_back({ .binding = binding, .stride = offset, .inputRate = VK_VERTEX_INPUT_RATE_VERTEX });
		}

		resources.attributeDescriptions = std::move(attributeDescriptions);
		resources.bindingDescriptions = std::move(bindingDescriptions);
	}

	// Parse uniform buffers
	for (const auto& resource: shaderResources.uniform_buffers)
	{
		uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
		uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

		// Check if we already have a binding for this set and binding number
		auto it = std::find_if(resources.descriptorSetLayoutBindings.begin(),
		        resources.descriptorSetLayoutBindings.end(),
		        [binding, set](const ShaderResources::DescriptorSetLayoutBinding& existingBinding) { return existingBinding.binding.binding == binding && existingBinding.set == set && existingBinding.binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; });

		if (it != resources.descriptorSetLayoutBindings.end())
		{
			// We found an existing binding, update its stage flags
			it->binding.stageFlags |= shaderModule.stage;
		}
		else
		{
			// Create a new binding
			VkDescriptorSetLayoutBinding layoutBinding{};
			layoutBinding.binding = binding;
			layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			layoutBinding.descriptorCount = 1;
			layoutBinding.stageFlags = shaderModule.stage;
			layoutBinding.pImmutableSamplers = nullptr; // Only relevant for samplers

			resources.descriptorSetLayoutBindings.push_back({ set, layoutBinding });
		}
	}

	// Parse combined image samplers
	for (const auto& resource: shaderResources.sampled_images)
	{
		uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
		uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

		// Check if we already have a binding for this set and binding number
		auto compareFn = [binding, set](const ShaderResources::DescriptorSetLayoutBinding& existingBinding)
		{
			return existingBinding.binding.binding == binding && existingBinding.set == set && existingBinding.binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		};

		auto it = std::find_if(resources.descriptorSetLayoutBindings.begin(), resources.descriptorSetLayoutBindings.end(), compareFn);
		if (it != resources.descriptorSetLayoutBindings.end())
		{
			// We found an existing binding, update its stage flags
			it->binding.stageFlags |= shaderModule.stage;
		}
		else
		{
			// Create a new binding
			VkDescriptorSetLayoutBinding layoutBinding{};
			layoutBinding.binding = binding;
			layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layoutBinding.descriptorCount = 1;
			layoutBinding.stageFlags = shaderModule.stage;
			layoutBinding.pImmutableSamplers = nullptr;

			resources.descriptorSetLayoutBindings.push_back({ set, layoutBinding });
		}
	}

	// Parse push constants
	uint32_t currentOffset = 0;
	for (const auto& resource: shaderResources.push_constant_buffers)
	{
		const auto& bufferType = compiler.get_type(resource.base_type_id);
		uint32_t size = compiler.get_declared_struct_size(bufferType);

		// Check if we already have a range for this offset
		auto it = std::find_if(resources.pushConstantRanges.begin(), resources.pushConstantRanges.end(), [currentOffset](const VkPushConstantRange& range) { return range.offset == currentOffset; });

		if (it != resources.pushConstantRanges.end())
		{
			// We found an existing range at this offset, update its stage flags and size
			it->stageFlags |= shaderModule.stage;
			it->size = std::max(it->size, size);
		}
		else
		{
			// Create a new range
			VkPushConstantRange range{};
			range.stageFlags = shaderModule.stage;
			range.offset = currentOffset;
			range.size = size;
			resources.pushConstantRanges.push_back(range);
		}

		currentOffset += size;
	}

	// Sort ranges by offset to ensure they're in the correct order
	std::sort(resources.pushConstantRanges.begin(), resources.pushConstantRanges.end(), [](const VkPushConstantRange& a, const VkPushConstantRange& b) { return a.offset < b.offset; });

	// Merge overlapping or adjacent ranges
	for (size_t i = 1; i < resources.pushConstantRanges.size(); /* no increment */)
	{
		auto& prev = resources.pushConstantRanges[i - 1];
		auto& curr = resources.pushConstantRanges[i];

		if (prev.offset + prev.size >= curr.offset)
		{
			// Ranges overlap or are adjacent, merge them
			prev.size = std::max(prev.size, curr.offset + curr.size - prev.offset);
			prev.stageFlags |= curr.stageFlags;
			resources.pushConstantRanges.erase(resources.pushConstantRanges.begin() + i);
		}
		else
		{
			++i;
		}
	}

	return resources;
}

ShaderManager::ShaderResources ShaderManager::CombineResources(const std::vector<ShaderModule>& shaderModules)
{
	ShaderResources combinedResources;

	for (const auto& shaderModule: shaderModules)
	{
		auto resources = ParseShader(shaderModule);

		// Merge attribute descriptions
		combinedResources.attributeDescriptions.insert(combinedResources.attributeDescriptions.end(), resources.attributeDescriptions.begin(), resources.attributeDescriptions.end());

		// Merge binding descriptions
		combinedResources.bindingDescriptions.insert(combinedResources.bindingDescriptions.end(), resources.bindingDescriptions.begin(), resources.bindingDescriptions.end());

		// Merge descriptor set layout bindings
		for (const auto& binding: resources.descriptorSetLayoutBindings)
		{
			auto it = std::find_if(combinedResources.descriptorSetLayoutBindings.begin(),
			        combinedResources.descriptorSetLayoutBindings.end(),
			        [&binding](const ShaderResources::DescriptorSetLayoutBinding& existingBinding) { return existingBinding.set == binding.set && existingBinding.binding.binding == binding.binding.binding && existingBinding.binding.descriptorType == binding.binding.descriptorType; });

			if (it != combinedResources.descriptorSetLayoutBindings.end())
			{
				// We found an existing binding, update its stage flags
				it->binding.stageFlags |= binding.binding.stageFlags;
			}
			else
			{
				// Create a new binding
				combinedResources.descriptorSetLayoutBindings.push_back(binding);
			}
		}

		// Merge push constant ranges
		combinedResources.pushConstantRanges.insert(combinedResources.pushConstantRanges.end(), resources.pushConstantRanges.begin(), resources.pushConstantRanges.end());
	}

	return combinedResources;
}

std::string HashBindings(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
	std::string hash = "descriptor_set_layout";
	for (const auto& binding: bindings)
	{
		hash += std::to_string(binding.binding) + std::to_string(binding.descriptorType) + std::to_string(binding.descriptorCount) + std::to_string(binding.stageFlags);
	}
	return hash;
}

std::vector<VkDescriptorSetLayout> ShaderManager::CreateDescriptorSetLayouts(const ShaderResources& resources)
{
	std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> setBindings;

	// Group bindings by set
	for (const auto& binding: resources.descriptorSetLayoutBindings)
	{
		setBindings[binding.set].push_back(binding.binding);
	}

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	descriptorSetLayouts.reserve(setBindings.size());

	for (const auto& [set, bindings]: setBindings)
	{
		std::string hash = HashBindings(bindings);

		if (m_descriptorSetLayouts.find(hash) != m_descriptorSetLayouts.end())
		{
			descriptorSetLayouts.push_back(m_descriptorSetLayouts[hash]);
			continue;
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		VkDescriptorSetLayout descriptorSetLayout;
		if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}

		m_descriptorSetLayouts[hash] = descriptorSetLayout;
		descriptorSetLayouts.push_back(descriptorSetLayout);

		spdlog::info("Created descriptor set layout for set {}", set);
		for (const auto& binding: bindings)
		{
			spdlog::info("  Binding {}: type {}, count {}, stage flags {}", binding.binding, static_cast<int>(binding.descriptorType), binding.descriptorCount, static_cast<int>(binding.stageFlags));
		}
	}

	return descriptorSetLayouts;
}

void ShaderManager::CleanupShaderModules()
{
	for (const auto& [path, shaderModule]: m_shaderModules)
	{
		vkDestroyShaderModule(m_device, shaderModule.handle, nullptr);
	}
	m_shaderModules.clear();
}

void ShaderManager::CleanupDescriptorSetLayouts()
{
	for (const auto& [hash, descriptorSetLayout]: m_descriptorSetLayouts)
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

	size_t fileSize = (size_t) file.tellg();
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
	file.close();

	return buffer;
}

VkShaderModule ShaderManager::CreateShaderModule(const std::vector<uint32_t>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size() * sizeof(uint32_t);
	createInfo.pCode = code.data();

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

uint32_t ShaderManager::GetFormatSize(VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return 16;
		case VK_FORMAT_R32G32B32_SFLOAT:
			return 12;
		case VK_FORMAT_R32G32_SFLOAT:
			return 8;
		case VK_FORMAT_R32_SFLOAT:
			return 4;
		// Add more cases as needed
		default:
			return 0; // Unknown format
	}
}
