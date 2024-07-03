//
// Created by alexm on 3/07/24.
//

#include "ShaderManager.h"
#include <fstream>
#include <stdexcept>

ShaderManager::ShaderManager(VkDevice device) : m_device(device) {}

ShaderManager::~ShaderManager() {
    cleanup();
}

std::pair<VkShaderModule, std::vector<uint32_t>&> ShaderManager::loadShader(const std::string& filename) {
    auto it = m_shaderModules.find(filename);
    if (it != m_shaderModules.end()) {
		return {it->second, m_shaderCodes[filename]};
    }

    std::vector<uint32_t> code = readFile(filename);
    VkShaderModule shaderModule = createShaderModule(code);
    m_shaderModules[filename] = shaderModule;
	m_shaderCodes[filename] = std::move(code);

	return {shaderModule, m_shaderCodes.at(filename)};
}

void ShaderManager::cleanup() {
    for (const auto& [filename, shaderModule] : m_shaderModules) {
        vkDestroyShaderModule(m_device, shaderModule, nullptr);
    }
    m_shaderModules.clear();
}

std::vector<uint32_t> ShaderManager::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();

    return buffer;
}

VkShaderModule ShaderManager::createShaderModule(const std::vector<uint32_t>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}