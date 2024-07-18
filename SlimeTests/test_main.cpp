#include <iostream>
#include "VulkanContext.h"
#include "spdlog/spdlog.h"
#include "ShaderManager.h"
#include "PipelineGenerator.h"
#include "ResourcePathManager.h"
#include "ModelManager.h"
#include <vector>

void testEngineInitialization() {
    ResourcePathManager resourcePathManager;
	ShaderManager shaderManager = ShaderManager();
	ModelManager modelManager = ModelManager(resourcePathManager);

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