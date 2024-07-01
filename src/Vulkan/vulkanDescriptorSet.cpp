#include "vulkanDescriptorSet.h"

#include "vulkanhelper.h"
#include "spdlog/spdlog.h"

namespace SlimeEngine
{
std::string IntTypeToString(VkDescriptorType type)
{
	switch (type)
	{
	case VK_DESCRIPTOR_TYPE_SAMPLER:
		return "VK_DESCRIPTOR_TYPE_SAMPLER";
	case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
	case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
	case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
		return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
	case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
	case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
		return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
	case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
	case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
		return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
	case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
		return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
	default:
		return "Unknown";
	}
}

int CreateDescriptorSetLayout(Init& init, RenderData& data, const std::vector<VkDescriptorSetLayoutBinding>& descriptorSetConfigs, const char* name)
{
	spdlog::info("Creating descriptor set layout: {}", name);

	auto bindings = std::vector<VkDescriptorSetLayoutBinding>();
	for (auto& config : descriptorSetConfigs)
	{
		spdlog::info("\tBinding: {}", std::to_string(config.binding));
		spdlog::info("\tType: {}", IntTypeToString(config.descriptorType));
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount                    = static_cast<uint32_t>(descriptorSetConfigs.size());
	layoutInfo.pBindings                       = descriptorSetConfigs.data();

	auto descriptorSetLayout = VkDescriptorSetLayout{};
	if (vkCreateDescriptorSetLayout(init.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		spdlog::error("Failed to create descriptor set layout!");
		return -1;
	}

	data.descriptorSetLayout[name] = descriptorSetLayout;

	return 0;
}

int CreateDefaultDescriptorSetLayouts(Init& init, RenderData& data)
{
	// triangle descriptor set layout
	std::vector<VkDescriptorSetLayoutBinding> triangleDescriptorSetLayoutBindings = {
		{
			.binding            = 0,
			.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount    = 1,
			.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = nullptr
		}
	};

	if (CreateDescriptorSetLayout(init, data, triangleDescriptorSetLayoutBindings, "triangle") != 0)
	{
		return -1;
	}

	// basic descriptor set layout
	std::vector<VkDescriptorSetLayoutBinding> basicDescriptorSetLayoutBindings = {
		{
			.binding            = 0,
			.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount    = 1,
			.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = nullptr
		},
		{
			.binding            = 1,
			.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount    = 1,
			.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr
		}
	};

	if (CreateDescriptorSetLayout(init, data, basicDescriptorSetLayoutBindings, "basic") != 0)
	{
		return -1;
	}

	return 0;
}

}