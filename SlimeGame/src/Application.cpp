#include "Application.h"

Application::Application()
      : m_window({ .title = "Slime Odyssey", .width = 1920, .height = 1080, .resizable = true, .decorated = true, .fullscreen = false }), m_scene(&m_window)
{
	InitializeLogging();
	InitializeWindow();
	InitializeEngine();
	InitializeManagers();
	InitializeScene();
}

void Application::Run()
{
	while (!m_window.ShouldClose())
	{
		float dt = m_window.Update();
		m_scene.Update(dt, m_engine, m_window.GetInputManager());
		m_scene.Render(m_engine, m_modelManager);
		m_engine.RenderFrame(m_modelManager, m_descriptorManager, &m_window, m_scene);
	}
}

void Application::Cleanup()
{
	m_scene.Exit(m_engine, m_modelManager);
	m_engine.Cleanup(m_shaderManager, m_modelManager, m_descriptorManager);
}

void Application::InitializeLogging()
{
	spdlog::set_level(spdlog::level::trace);
	spdlog::stdout_color_mt("console");
}

void Application::InitializeWindow()
{
	m_window.SetResizeCallback([this](int width, int height) { m_engine.CreateSwapchain(&m_window); });
}

void Application::InitializeEngine()
{
	if (m_engine.CreateEngine(&m_window) != 0)
	{
		throw std::runtime_error("Failed to create engine");
	}
}

void Application::InitializeManagers()
{
	m_resourcePathManager = ResourcePathManager();
	m_shaderManager = ShaderManager();
	m_modelManager = ModelManager(m_resourcePathManager);
	m_descriptorManager = DescriptorManager(m_engine.GetDevice());
}

void Application::InitializeScene()
{
	m_scene = PlatformerGame(&m_window);
	if (m_scene.Enter(m_engine, m_modelManager, m_shaderManager, m_descriptorManager) != 0)
	{
		throw std::runtime_error("Failed to initialize scene");
	}
}
