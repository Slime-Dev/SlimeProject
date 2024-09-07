#include "ModelManager.h"
#include <spdlog/spdlog.h>
#include <functional>
#include <vector>
#include <stdexcept>

struct TestResult {
    std::string testName;
    bool passed;
    std::string errorMessage;
};

TestResult RunTest(const std::string& testName, std::function<void(ModelManager& modelManager)> testFunction) {
    ModelManager modelManager = ModelManager();
    try {
        testFunction(modelManager);
        return { testName, true, "" };
    } catch (const std::exception& e) {
        return { testName, false, e.what() };
    } catch (...) {
        return { testName, false, "Unknown exception occurred" };
    }
}

void LoadExistingModels(ModelManager& modelManager) {
    auto bunnyMesh = modelManager.LoadModel("stanford-bunny.obj", "basic");
    if (bunnyMesh == nullptr || bunnyMesh->vertices.empty() || bunnyMesh->indices.empty()) {
        throw std::runtime_error("Failed to load model 'stanford-bunny.obj'");
    }

    auto monkeyMesh = modelManager.LoadModel("suzanne.obj", "basic");
    if (monkeyMesh == nullptr || monkeyMesh->vertices.empty() || monkeyMesh->indices.empty()) {
        throw std::runtime_error("Failed to load model 'suzanne.obj'");
    }

    auto cubeMesh = modelManager.LoadModel("cube.obj", "basic");
    if (cubeMesh == nullptr || cubeMesh->vertices.empty() || cubeMesh->indices.empty()) {
        throw std::runtime_error("Failed to load model 'cube.obj'");
    }
}

void TestShapeCreation(ModelManager& modelManager) {
    auto plane = modelManager.CreatePlane(nullptr, 1.0f, 10);
    if (plane == nullptr || plane->vertices.empty() || plane->indices.empty()) {
        throw std::runtime_error("Failed to create plane");
    }

    auto cube = modelManager.CreateCube(nullptr, 1.0f);
    if (cube == nullptr || cube->vertices.empty() || cube->indices.empty()) {
        throw std::runtime_error("Failed to create cube");
    }

    auto sphere = modelManager.CreateSphere(nullptr, 1.0f, 20, 20);
    if (sphere == nullptr || sphere->vertices.empty() || sphere->indices.empty()) {
        throw std::runtime_error("Failed to create sphere");
    }

    auto cylinder = modelManager.CreateCylinder(nullptr, 1.0f, 2.0f, 20);
    if (cylinder == nullptr || cylinder->vertices.empty() || cylinder->indices.empty()) {
        throw std::runtime_error("Failed to create cylinder");
    }
}

void TestTextureLoading(ModelManager& modelManager) {
    // Note: This is a simplified test as we can't fully test texture loading without a Vulkan context
    auto texture = modelManager.GetTexture("test_texture.png");
    if (texture != nullptr) {
        throw std::runtime_error("Unexpectedly found non-existent texture");
    }
}

int main() {
    std::vector<TestResult> testResults;

    testResults.push_back(RunTest("LoadExistingModels", LoadExistingModels));
    testResults.push_back(RunTest("TestShapeCreation", TestShapeCreation));
    testResults.push_back(RunTest("TestTextureLoading", TestTextureLoading));

    bool allPassed = true;
    for (const auto& result : testResults) {
        if (result.passed) {
            spdlog::info("Test '{}' passed.", result.testName);
        } else {
            spdlog::error("Test '{}' failed with error: {}", result.testName, result.errorMessage);
            allPassed = false;
        }
    }

    if (allPassed) {
        spdlog::info("All Tests Passed!");
        return 0;
    } else {
        spdlog::error("Some Tests Failed.");
        return 1;
    }
}