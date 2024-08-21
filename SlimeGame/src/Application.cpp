#include "Application.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <DescriptorManager.h>

#include "spdlog/sinks/base_sink.h"
#include "spdlog/spdlog.h"
#include <ComponentInspector.h>

#include <iostream>


Application::Application()
      : m_window({ .title = "Slime Odyssey", .width = 1920, .height = 1080, .resizable = true, .decorated = true, .fullscreen = false }), m_scene(&m_window)
{
	InitializeLogging();
	InitializeWindow();
	InitializeVulkanContext();
	InitializeManagers();
	
	ComponentInspector::RegisterComponentInspectors();

	InitializeScene();
}

void Application::Run()
{
	while (!m_window.ShouldClose())
	{
		float dt = m_window.Update();
		m_scene.Update(dt, m_vulkanContext, m_window.GetInputManager());
		m_vulkanContext.RenderFrame(m_modelManager, &m_window, &m_scene);
	}
}

void Application::Cleanup()
{
	m_scene.Exit(m_vulkanContext, m_modelManager);
	m_vulkanContext.Cleanup(m_modelManager);
}

void Application::InitializeLogging()
{
    spdlog::set_level(spdlog::level::trace);
}

void Application::InitializeWindow()
{
	m_window.SetResizeCallback([this](int width, int height) { m_vulkanContext.CreateSwapchain(&m_window); });
}

void Application::InitializeVulkanContext()
{
	if (m_vulkanContext.CreateContext(&m_window, &m_modelManager) != 0)
	{
		throw std::runtime_error("Failed to create Vulkan Context!");
	}
}

void Application::InitializeManagers()
{
	m_modelManager = ModelManager();
}

void Application::InitializeScene()
{
	if (m_scene.Enter(m_vulkanContext, m_modelManager) != 0)
	{
		throw std::runtime_error("Failed to initialize scene");
	}
}
