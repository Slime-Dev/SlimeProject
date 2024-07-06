//
// Created by alexm on 4/07/24.
//

#include "ResourcePathManager.h"

ResourcePathManager::ResourcePathManager()
{
	m_rootDirectory = SetRootDirectory();
	InitializeDirectories();
}

std::string ResourcePathManager::GetResourcePath(ResourceType type, const std::string& resourceName) const
{
	auto it = m_directories.find(type);
	if (it == m_directories.end())
	{
		throw std::runtime_error("Invalid resource type");
	}
	return it->second + "/" + resourceName;
}

std::string ResourcePathManager::GetRootDirectory()
{
	return m_rootDirectory;
}

std::string ResourcePathManager::GetShaderPath(const std::string& shaderName) const
{
	return GetResourcePath(ResourceType::Shader, shaderName);
}

std::string ResourcePathManager::GetModelPath(const std::string& modelName) const
{
	return GetResourcePath(ResourceType::Model, modelName);
}

std::string ResourcePathManager::GetTexturePath(const std::string& textureName) const
{
	return GetResourcePath(ResourceType::Texture, textureName);
}

std::string ResourcePathManager::GetSoundPath(const std::string& soundName) const
{
	return GetResourcePath(ResourceType::Sound, soundName);
}

std::string ResourcePathManager::GetScriptPath(const std::string& scriptName) const
{
	return GetResourcePath(ResourceType::Script, scriptName);
}

std::string ResourcePathManager::GetConfigPath(const std::string& configName) const
{
	return GetResourcePath(ResourceType::Config, configName);
}

std::string ResourcePathManager::SetRootDirectory()
{
	auto srcPath = std::filesystem::absolute(__FILE__);
	return std::filesystem::path(srcPath).parent_path().parent_path().string() + "/resources";
}

void ResourcePathManager::InitializeDirectories()
{
	m_directories[ResourceType::Shader]  = m_rootDirectory + "/shaders";
	m_directories[ResourceType::Model]   = m_rootDirectory + "/models";
	m_directories[ResourceType::Texture] = m_rootDirectory + "/textures";
	m_directories[ResourceType::Sound]   = m_rootDirectory + "/sounds";
	m_directories[ResourceType::Script]  = m_rootDirectory + "/scripts";
	m_directories[ResourceType::Config]  = m_rootDirectory + "/config";

	// Create directories if they don't exist
	for (const auto& [type, path] : m_directories)
	{
		std::filesystem::create_directories(path);
	}
}