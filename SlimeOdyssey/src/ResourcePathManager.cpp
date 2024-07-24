#include "ResourcePathManager.h"
#ifdef _WIN32
	#include <direct.h>
#else
	#include <unistd.h>
#endif

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
    char* path = nullptr;
    #ifdef _WIN32
        path = _getcwd(nullptr, 0);
    #else
        path = getcwd(nullptr, 0);
    #endif
    
    if (path == nullptr) {
        throw std::runtime_error("Failed to get current working directory");
    }
    
    std::string currentPath(path);
    free(path);
    
    return currentPath + "/resources";
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
