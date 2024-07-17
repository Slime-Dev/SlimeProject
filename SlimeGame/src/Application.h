#pragma once

#include <spdlog/sinks/stdout_color_sinks.h>

#include "ModelManager.h"
#include "PlatformerGame.h"
#include "ResourcePathManager.h"
#include "ShaderManager.h"
#include "VulkanContext.h"

class Application
{
public:
	Application();
	void Run();
	void Cleanup();

private:
	void InitializeLogging();
	void InitializeWindow();
	void InitializeEngine();
	void InitializeManagers();
	void InitializeScene();

	SlimeWindow m_window;
	VulkanContext m_engine;
	ResourcePathManager m_resourcePathManager;
	ShaderManager m_shaderManager;
	ModelManager m_modelManager;
	DescriptorManager m_descriptorManager;
	PlatformerGame m_scene;
};
