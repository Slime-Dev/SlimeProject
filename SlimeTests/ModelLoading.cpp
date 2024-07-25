#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include "ResourcePathManager.h"
#include "ModelManager.h"
#include <spdlog/spdlog.h>

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

void LoadBunny(ModelManager& modelManager)
{
    auto bunnyMesh = modelManager.LoadModel("stanford-bunny.obj", "basic");
    if (bunnyMesh == nullptr) {
        throw std::runtime_error("Failed to load model 'stanford-bunny.obj'");
    }

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
    std::vector<TestResult> testResults;
    testResults.push_back(RunTest("LoadBunny", LoadBunny));
    testResults.push_back(RunTest("LoadMonkey", LoadMonkey));
    testResults.push_back(RunTest("LoadCube", LoadCube));

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
