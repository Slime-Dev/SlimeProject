#include <iostream>
#include "ResourcePathManager.h"
#include "ModelManager.h"
#include <vector>

void testEngineInitialization() {
    ResourcePathManager resourcePathManager;
    ModelManager modelManager = ModelManager(resourcePathManager);

    auto bunnyMesh = modelManager.LoadModel("Stanford-bunny.obj", "basic");
    assert(bunnyMesh != NULL);

    assert(bunnyMesh->vertices.size() != 0);

    assert(bunnyMesh->indices.size() != 0);

    auto suzanneMesh = modelManager.LoadModel("suzanne.obj", "basic");
    assert(suzanneMesh != NULL);

    assert(suzanneMesh->vertices.size() != 0);

    assert(suzanneMesh->indices.size() != 0);

    auto cubeMesh = modelManager.LoadModel("cube.obj", "basic");
    assert(cubeMesh != NULL);

    assert(cubeMesh->vertices.size() != 0);

    assert(cubeMesh->indices.size() != 0);
}

int main() {
    try {
        testEngineInitialization();
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception occurred during testing." << std::endl;
        return 2;
    }
    std::cout << "All tests passed!" << std::endl;
    return 0;
}