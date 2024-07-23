#include "ResourcePathManager.h"

std::string ResourcePathManager::s_rootDirectory;
std::unordered_map<ResourcePathManager::ResourceType, std::string> ResourcePathManager::s_directories;

std::string ResourcePathManager::GetResourcePath(ResourceType type, const std::string& resourceName)
{
	if (s_directories.empty())
	{
		s_rootDirectory = SetRootDirectory();
		InitializeDirectories();
	}

	auto it = s_directories.find(type);
	if (it == s_directories.end())
	{
		throw std::runtime_error("Invalid resource type");
	}
	return it->second + "/" + resourceName;
}

std::string ResourcePathManager::GetRootDirectory()
{
	if (s_rootDirectory.empty())
	{
		s_rootDirectory = SetRootDirectory();
	}
	return s_rootDirectory;
}

std::string ResourcePathManager::GetShaderPath(const std::string& shaderName)
{
	return GetResourcePath(ResourceType::Shader, shaderName);
}

std::string ResourcePathManager::GetModelPath(const std::string& modelName)
{
	return GetResourcePath(ResourceType::Model, modelName);
}

std::string ResourcePathManager::GetTexturePath(const std::string& textureName)
{
	return GetResourcePath(ResourceType::Texture, textureName);
}

std::string ResourcePathManager::GetSoundPath(const std::string& soundName)
{
	return GetResourcePath(ResourceType::Sound, soundName);
}

std::string ResourcePathManager::GetScriptPath(const std::string& scriptName)
{
	return GetResourcePath(ResourceType::Script, scriptName);
}

std::string ResourcePathManager::GetConfigPath(const std::string& configName)
{
	return GetResourcePath(ResourceType::Config, configName);
}

std::string ResourcePathManager::GetFontPath(const std::string& fontName)
{
	return GetResourcePath(ResourceType::Font, fontName);
}

std::string ResourcePathManager::SetRootDirectory()
{
	auto srcPath = std::filesystem::absolute(__FILE__);
	return std::filesystem::path(srcPath).parent_path().parent_path().string() + "/resources";
}

void ResourcePathManager::InitializeDirectories()
{
	s_directories[ResourceType::Shader] = s_rootDirectory + "/shaders";
	s_directories[ResourceType::Model] = s_rootDirectory + "/models";
	s_directories[ResourceType::Texture] = s_rootDirectory + "/textures";
	s_directories[ResourceType::Sound] = s_rootDirectory + "/sounds";
	s_directories[ResourceType::Script] = s_rootDirectory + "/scripts";
	s_directories[ResourceType::Config] = s_rootDirectory + "/config";
	s_directories[ResourceType::Font] = s_rootDirectory + "/fonts";

	// Create directories if they don't exist
	for (const auto& [type, path]: s_directories)
	{
		std::filesystem::create_directories(path);
	}
}
