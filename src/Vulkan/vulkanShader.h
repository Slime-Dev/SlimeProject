#pragma once
#include "vulkanGraphicsPipeline.h"
#include <VkBootstrap.h>
#include <string>

namespace SlimeEngine
{
struct RenderData;
struct Init;

struct ShaderConfig
{
	std::string vertShaderPath;
	std::string fragShaderPath;

	VertexInputConfig vertexInputConfig;
};



}