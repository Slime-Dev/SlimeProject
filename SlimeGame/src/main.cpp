#include "scene.h"

int main()
{
	SlimeWindow::WindowProps windowProps{
		.title = "Slime Odyssey",
		.width = 1920,
		.height = 1080,
		.resizable = true,
		.decorated = true,
		.fullscreen = false
	};

	SlimeWindow window(windowProps);

	Engine engine(&window);

	if (engine.CreateEngine() != 0)
		return -1;

	auto resizeCallback = [&](int width, int height) {
		engine.CreateSwapchain();
	};
	window.SetResizeCallback(resizeCallback);

	Scene scene(engine);
	if (scene.Setup() != 0)
		return -1;


	float dt = 0.0f;
	// Main loop
	while (!window.ShouldClose())
	{
		dt = window.Update();
		scene.Update(dt, engine.GetInputManager());
		scene.Render();
		engine.RenderFrame();
	}

	return 0;
}