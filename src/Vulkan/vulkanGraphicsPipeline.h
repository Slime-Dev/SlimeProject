#pragma once
#include <VkBootstrap.h>

namespace SlimeEngine
{
struct ShaderConfig;
struct RenderData;
struct Init;

struct PipelineConfig
{
	VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	bool primitiveRestartEnable  = false;
	float lineWidth              = 1.0f;
	VkCullModeFlags cullMode     = VK_CULL_MODE_BACK_BIT;
	VkFrontFace frontFace        = VK_FRONT_FACE_CLOCKWISE;
	bool depthClampEnable        = false;
	bool rasterizerDiscardEnable = false;
};

struct ColorBlendingConfig
{
	VkBool32 blendEnable                 = VK_FALSE;
	VkLogicOp logicOp                    = VK_LOGIC_OP_COPY;
	VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
};

struct VertexInputConfig
{
	VkVertexInputBindingDescription bindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
};

int CreateGraphicsPipeline(Init& init, RenderData& data, const char* name, const ShaderConfig& shaderConfig, const PipelineConfig& config, const ColorBlendingConfig& blendingConfig);

}