#include "ResourcePathManager.h"
#include <spdlog/spdlog.h>

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
   
    // The application can have many places where it is run from so we set multiple options
    const char* possibleSubDirs[] = {
        "/bin/Release/resources",
        "/bin/Debug/resources",
        "/bin/resources",
        "/build/resources",
        "/resources",
        "/assets",
        "/data",
        "/../bin/resources",
        "/../bin/assets",
        "/../bin/data",
		"/../bin/Release/resources",
		"/../bin/Debug/resources",
		"/../resources",
        "/../assets",
        "/../data",
        "/bin/x64/Release/resources",
        "/bin/x64/Debug/resources",
        "/bin/x86/Release/resources",
        "/bin/x86/Debug/resources",
        "/out/build/x64-Release/resources",
        "/out/build/x64-Debug/resources",
        "/out/build/x86-Release/resources",
        "/out/build/x86-Debug/resources",
        "/build/Release/resources",
        "/build/Debug/resources"
    };

    for (const char* subDir : possibleSubDirs)
    {
        std::string currentPath = std::string(path) + subDir;
        if (std::filesystem::exists(currentPath))
        {
			spdlog::info("Found resources directory at: {}", currentPath);
            free(path);
            return currentPath;
        }
    }

	spdlog::warn("Failed to find resources directory, using current working directory");

    // If none of the above paths exist, default to the original fallback
    std::string currentPath = std::string(path) + "/resources";
    free(path);
    return currentPath;
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
