#include <iostream>
#include "ResourcePathManager.h"
#include "ModelManager.h"
#include <spdlog/spdlog.h>
#include <vector>

void LoadBunny() {
    ResourcePathManager resourcePathManager;
    ModelManager modelManager = ModelManager(resourcePathManager);
    auto bunnyMesh = modelManager.LoadModel("Stanford-bunny.obj", "basic");
    assert(bunnyMesh != NULL);

    assert(bunnyMesh->vertices.size() != 0);

    assert(bunnyMesh->indices.size() != 0);
}

void LoadMonkey() {
    ResourcePathManager resourcePathManager;
    ModelManager modelManager = ModelManager(resourcePathManager);
    auto suzanneMesh = modelManager.LoadModel("suzanne.obj", "basic");
    assert(suzanneMesh != NULL);

    assert(suzanneMesh->vertices.size() != 0);

    assert(suzanneMesh->indices.size() != 0);
}

void LoadCube() {
    ResourcePathManager resourcePathManager;
    ModelManager modelManager = ModelManager(resourcePathManager);
    auto cubeMesh = modelManager.LoadModel("cube.obj", "basic");
    assert(cubeMesh != NULL);

    assert(cubeMesh->vertices.size() != 0);

    assert(cubeMesh->indices.size() != 0);
}


int main() {
    try {
        LoadBunny();
        LoadMonkey();
        LoadCube();
    }
    catch (const std::exception& e) {
        spdlog::info("Test failed with exception: {}", e.what());
        return 1;
    }
    catch (...) {
        spdlog::info("Unknown exception occurred during testing.");
        return 2;
    }
    spdlog::info("All Tests Passed!");
    return 0;
}