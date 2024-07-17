#include "Engine.h"
#include "ModelManager.h"
#include "PlatformerGame.h"
#include "ResourcePathManager.h"
#include "ShaderManager.h"

int main()
{
	SlimeWindow::WindowProps windowProps{ .title = "Slime Odyssey", .width = 1920, .height = 1080, .resizable = true, .decorated = true, .fullscreen = false };

	SlimeWindow window(windowProps);
	InputManager* inputManager = window.GetInputManager();

	Engine engine;

	if (engine.CreateEngine(&window) != 0)
		return -1;

	ResourcePathManager resourcePathManager;
	ShaderManager shaderManager = ShaderManager();
	ModelManager modelManager = ModelManager(resourcePathManager);
	DescriptorManager descriptorManager = DescriptorManager(engine.GetDevice());

	auto resizeCallback = [&](int width, int height) { engine.CreateSwapchain(&window); };
	window.SetResizeCallback(resizeCallback);

	PlatformerGame scene(&window);
	if (scene.Enter(engine, modelManager, shaderManager, descriptorManager) != 0)
		return -1;

	float dt = 0.0f;
	// Main loop
	while (!window.ShouldClose())
	{
		dt = window.Update();
		scene.Update(dt, engine, inputManager);
		scene.Render(engine, modelManager);
		engine.RenderFrame(modelManager, descriptorManager, &window, scene);
	}
	scene.Exit(engine, modelManager);
	engine.Cleanup(shaderManager, modelManager, descriptorManager);

	return 0;
}
