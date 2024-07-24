#include <iostream>
#include <stdexcept>
#include "ModelManager.h"
#include <spdlog/spdlog.h>
#include <vector>

void LoadBunny(ModelManager& modelManager)
{
    auto bunnyMesh = modelManager.LoadModel("stanford-bunny.obj", "basic");
    if (bunnyMesh->vertices.size() == 0) {
        throw std::runtime_error("Model 'stanford-bunny.obj' has no vertices");
    }

    if (bunnyMesh->indices.size() == 0) {
        throw std::runtime_error("Model 'stanford-bunny.obj' has no indices");
    }
}

void LoadMonkey(ModelManager& modelManager)
{
    auto suzanneMesh = modelManager.LoadModel("suzanne.obj", "basic");
    if (suzanneMesh == nullptr) {
        throw std::runtime_error("Failed to load model 'suzanne.obj'");
    }

    if (suzanneMesh->vertices.size() == 0) {
        throw std::runtime_error("Model 'suzanne.obj' has no vertices");
    }

    if (suzanneMesh->indices.size() == 0) {
        throw std::runtime_error("Model 'suzanne.obj' has no indices");
    }
}

void LoadCube(ModelManager& modelManager)
{
    auto cubeMesh = modelManager.LoadModel("cube.obj", "basic");
    if (cubeMesh == nullptr) {
        throw std::runtime_error("Failed to load model 'cube.obj'");
    }

    if (cubeMesh->vertices.size() == 0) {
        throw std::runtime_error("Model 'cube.obj' has no vertices");
    }

    if (cubeMesh->indices.size() == 0) {
        throw std::runtime_error("Model 'cube.obj' has no indices");
    }
}

int main() {
	ModelManager modelManager = ModelManager();

    try {
		LoadBunny(modelManager);
		LoadMonkey(modelManager);
		LoadCube(modelManager);
    }
    catch (const std::exception& e) {
        spdlog::error("Test failed with exception: {}", e.what());
        return 1;
    }
    catch (...) {
        spdlog::error("Unknown exception occurred during testing.");
        return 2;
    }
    spdlog::info("All Tests Passed!");
    return 0;
}
