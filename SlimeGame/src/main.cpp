#include "scene.h"

int main()
{
    Engine engine("Slime Odyssey", 800, 600);

    if (engine.DeviceInit() != 0)
        return -1;
    if (engine.CreateCommandPool() != 0)
        return -1;
    if (engine.GetQueues() != 0)
        return -1;
    if (engine.CreateSwapchain() != 0)
        return -1;

    if (SetupScean(engine) != 0)
        return -1;

    if (engine.CreateRenderCommandBuffers() != 0)
        return -1;
    if (engine.InitSyncObjects() != 0)
        return -1;

    Window& window = engine.GetWindow();

    // Main loop
    while (!window.ShouldClose())
    {
        window.PollEvents();
        engine.RenderFrame();
    }

    return 0;
}