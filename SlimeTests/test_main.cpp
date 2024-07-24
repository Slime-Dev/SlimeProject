#include <iostream>
#include <stdexcept>
#include "ResourcePathManager.h"
#include "ModelManager.h"
#include <spdlog/spdlog.h>
#include <vector>

void LoadBunny() {
    ModelManager modelManager = ModelManager();
    // Upper case for Windows
    auto bunnyMesh = modelManager.LoadModel("Stanford-bunny.obj", "basic");
    if (bunnyMesh == nullptr) {
        // Lower case for Linux
        bunnyMesh = modelManager.LoadModel("stanford-bunny.obj", "basic");
        if (bunnyMesh == nullptr) {
            throw std::runtime_error("Failed to load model 'Stanford-bunny.obj'");
        }
    }

    if (bunnyMesh->vertices.size() == 0) {
        throw std::runtime_error("Model 'Stanford-bunny.obj' has no vertices");
    }

    if (bunnyMesh->indices.size() == 0) {
        throw std::runtime_error("Model 'stanford-bunny.obj' has no indices");
    }
}

void LoadMonkey() {
    ModelManager modelManager = ModelManager();
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

void LoadCube() {
    ModelManager modelManager = ModelManager();
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
    try {
        LoadBunny();
        LoadMonkey();
        LoadCube();
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
