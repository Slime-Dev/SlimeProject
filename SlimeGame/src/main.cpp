#include "Engine.h"
#include "ModelManager.h"
#include "PlatformerGame.h"
#include "ResourcePathManager.h"
#include "ShaderManager.h"

int main()
{
	SlimeWindow::WindowProps windowProps{ .title = "Slime Odyssey", .width = 1920, .height = 1080, .resizable = true, .decorated = true, .fullscreen = false };

	SlimeWindow window(windowProps);

	Engine engine(&window);

	if (engine.CreateEngine() != 0)
		return -1;

	ResourcePathManager resourcePathManager;
	ShaderManager shaderManager = ShaderManager();
	ModelManager modelManager = ModelManager(resourcePathManager);
	DescriptorManager descriptorManager = DescriptorManager(engine.GetDevice());

	engine.SetupTesting(modelManager, descriptorManager); // TODO: REMOVE THIS

	auto resizeCallback = [&](int width, int height) { engine.CreateSwapchain(); };
	window.SetResizeCallback(resizeCallback);

	PlatformerGame scene(engine);
	if (scene.Setup(modelManager, shaderManager, descriptorManager) != 0)
		return -1;

	float dt = 0.0f;
	// Main loop
	while (!window.ShouldClose())
	{
		dt = window.Update();
		scene.Update(dt, engine.GetInputManager());
		scene.Render();
		engine.RenderFrame(modelManager, descriptorManager);
	}

	engine.Cleanup(shaderManager, modelManager, descriptorManager);

	return 0;
}
