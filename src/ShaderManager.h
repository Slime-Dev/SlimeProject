//
// Created by alexm on 3/07/24.
//

#pragma once

#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <vector>

class ShaderManager
{
public:
	ShaderManager() = default;
	ShaderManager(VkDevice device);
	~ShaderManager();

	std::pair<VkShaderModule, std::vector<uint32_t>&> loadShader(const std::string& filename);

	void cleanup();

private:
	VkDevice m_device;
	std::unordered_map<std::string, VkShaderModule> m_shaderModules;
	std::unordered_map<std::string, std::vector<uint32_t>> m_shaderCodes;

	std::vector<uint32_t> readFile(const std::string& filename);
	VkShaderModule createShaderModule(const std::vector<uint32_t>& code);
};