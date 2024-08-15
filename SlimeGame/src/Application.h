#pragma once

#include <ModelManager.h>
#include <ShaderManager.h>
#include <VulkanContext.h>
#include <SlimeWindow.h>

#define DEBUG_SCENE


#ifdef DEBUG_SCENE
#	include "DebugScene.h"
#else
#	include "PlatformerGame.h"
#endif

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
	ModelManager m_modelManager;

#ifdef DEBUG_SCENE
	DebugScene m_scene;
#else
	PlatformerGame m_scene;
#endif
};
