#include "scene.h"

int main()
{
	Engine engine("Slime Odyssey", 1920, 1080, true);

	if (engine.CreateEngine() != 0)
		return -1;

	Scene scene(engine);
	if (scene.Setup() != 0)
		return -1;

	Window& window = engine.GetWindow();

	// Main loop
	while (!window.ShouldClose())
	{
		window.PollEvents();
		engine.GetInputManager().Update();
		scene.Update(window.GetDeltaTime());
		scene.Render();
		engine.RenderFrame();
	}

	return 0;
}
