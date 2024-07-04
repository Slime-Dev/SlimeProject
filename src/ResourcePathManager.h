//
// Created by alexm on 4/07/24.
//

#pragma once

#include <string>
#include <filesystem>
#include <unordered_map>
#include <stdexcept>

class ResourcePathManager
{
public:
	enum class ResourceType
	{
		Shader,
		Model,
		Texture,
		Sound,
		Script,
		Config
	};

	ResourcePathManager();

	std::string GetResourcePath(ResourceType type, const std::string& resourceName) const;
	std::string GetRootDirectory();

	// Helper methods for specific resource types
	std::string GetShaderPath(const std::string& shaderName) const;
	std::string GetModelPath(const std::string& modelName) const;
	std::string GetTexturePath(const std::string& textureName) const;
	std::string GetSoundPath(const std::string& soundName) const;
	std::string GetScriptPath(const std::string& scriptName) const;
	std::string GetConfigPath(const std::string& configName) const;

private:
	std::string m_rootDirectory;
	std::unordered_map<ResourceType, std::string> m_directories;

	std::string SetRootDirectory();
	void InitializeDirectories();
};