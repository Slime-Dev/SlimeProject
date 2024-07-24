#pragma once

#include <ModelManager.h>
#include <ShaderManager.h>
#include <VulkanContext.h>

#include "PlatformerGame.h"

class Application
{
public:
	Application();
	void Run();
	void Cleanup();

private:
	void InitializeLogging();
	void InitializeWindow();
	void InitializeVulkanContext();
	void InitializeManagers();
	void InitializeScene();

	SlimeWindow m_window;
	VulkanContext m_vulkanContext;
	ShaderManager m_shaderManager;
	ModelManager m_modelManager;
	DescriptorManager* m_descriptorManager = nullptr;
	PlatformerGame m_scene;
};
