#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

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
		Config,
		Font
	};

	static std::string GetResourcePath(ResourceType type, const std::string& resourceName);
	static std::string GetRootDirectory();

	// Helper methods for specific resource types
	static std::string GetShaderPath(const std::string& shaderName);
	static std::string GetModelPath(const std::string& modelName);
	static std::string GetTexturePath(const std::string& textureName);
	static std::string GetSoundPath(const std::string& soundName);
	static std::string GetScriptPath(const std::string& scriptName);
	static std::string GetConfigPath(const std::string& configName);
	static std::string GetFontPath(const std::string& fontName);

private:
	static std::string s_rootDirectory;
	static std::unordered_map<ResourceType, std::string> s_directories;

	static std::string SetRootDirectory();
	static void InitializeDirectories();
};
