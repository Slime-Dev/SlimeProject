#include "Application.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <DescriptorManager.h>

#include "spdlog/sinks/base_sink.h"
#include "spdlog/spdlog.h"

#include <iostream>

#ifdef _WIN32
#	include <intrin.h>
#else
#	include <csignal>
#endif

template<typename Mutex>
class breakpoint_on_error_sink : public spdlog::sinks::base_sink<Mutex>
{
protected:
	const char* get_color_code(spdlog::level::level_enum level) const
	{
		switch (level)
		{
			case spdlog::level::trace:
				return "\033[90m"; // Dark gray
			case spdlog::level::debug:
				return "\033[36m"; // Cyan
			case spdlog::level::info:
				return "\033[32m"; // Green
			case spdlog::level::warn:
				return "\033[33m"; // Yellow
			case spdlog::level::err:
				return "\033[31m"; // Red
			case spdlog::level::critical:
				return "\033[1m\033[31m"; // Bold red
			default:
				return "\033[0m"; // Reset color
		}
	}

	void sink_it_(const spdlog::details::log_msg& msg) override
	{
		// Get current time
		auto now = std::chrono::system_clock::now();
		auto time = std::chrono::system_clock::to_time_t(now);
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

		// Format time
		std::tm tm_time{};
#ifdef _WIN32
		localtime_s(&tm_time, &time);
#else
		localtime_r(&time, &tm_time);
#endif

        // Print timestamp
		std::cout << '[' << std::put_time(&tm_time, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ms.count() << "] ";

		// Print colored log level
		std::cout << '[' << get_color_code(msg.level) << spdlog::level::to_string_view(msg.level).data() << "\033[0m] "; // Reset color after log level

		// Print message
		std::cout.write(msg.payload.data(), static_cast<std::streamsize>(msg.payload.size()));

		// End line
		std::cout << std::endl;

		// Check if the log level is error
		if (msg.level == spdlog::level::err || msg.level == spdlog::level::critical)
		{
			// Trigger a breakpoint
#ifdef _WIN32
			__debugbreak();
#else
			std::raise(SIGTRAP);
#endif
		}
	}

	void flush_() override
	{
	}
};


Application::Application()
      : m_window({ .title = "Slime Odyssey", .width = 1920, .height = 1080, .resizable = true, .decorated = true, .fullscreen = false }), m_scene(&m_window)
{
	InitializeLogging();
	InitializeWindow();
	InitializeVulkanContext();
	InitializeManagers();
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
	auto breakpoint_sink = std::make_shared<breakpoint_on_error_sink<std::mutex>>();
    spdlog::set_default_logger(std::make_shared<spdlog::logger>("console", breakpoint_sink));
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
