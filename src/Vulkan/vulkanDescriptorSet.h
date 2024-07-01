#pragma once
#include <VkBootstrap.h>

namespace SlimeEngine
{
struct RenderData;
struct Init;

int CreateDescriptorSetLayout(Init& init, RenderData& data, const std::vector<VkDescriptorSetLayoutBinding>& descriptorSetConfigs, const char* name);

int CreateDefaultDescriptorSetLayouts(Init& init, RenderData& data);

}