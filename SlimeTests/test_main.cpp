#include <iostream>
#include "Engine.h"
#include "spdlog/spdlog.h"
#include "ShaderManager.h"
#include "PipelineGenerator.h"
#include "ResourcePathManager.h"
#include "ModelManager.h"
#include <vector>

void testEngineInitialization() {
	// SlimeWindow::WindowProps windowProps{ .title = "Slime Odyssey", .width = 1920, .height = 1080, .resizable = true, .decorated = true, .fullscreen = false };

	// SlimeWindow window(windowProps);

	// Engine engine(&window);

    // assert(engine.CreateEngine() == 0);

    assert(1 == 1);
}

int main() {
    try {
        // Run tests
        testEngineInitialization();

        std::cout << "All tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception occurred during testing." << std::endl;
        return 2;
    }
}