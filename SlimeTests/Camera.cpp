#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/epsilon.hpp>
#include <stdexcept>
#include <cmath>
#include <iostream>
#include <functional>
#include <spdlog/spdlog.h>

struct TestResult {
    std::string testName;
    bool passed;
    std::string errorMessage;
};

// Helper function for floating point comparisons
bool isEqual(const glm::mat4& a, const glm::mat4& b, float epsilon = 1e-6f) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (std::abs(a[i][j] - b[i][j]) > epsilon) {
                return false;
            }
        }
    }
    return true;
}

void TestInitialState(Camera& camera) {
    if (camera.GetPosition() != glm::vec3(0.0f, 0.0f, 1.0f)) {
        throw std::runtime_error("Initial position is incorrect");
    }

    glm::mat4 expectedView = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    if (!isEqual(camera.GetViewMatrix(), expectedView)) {
        throw std::runtime_error("Initial view matrix is incorrect");
    }

    glm::mat4 expectedProjection = glm::perspective(glm::radians(45.0f), 1.778f, 0.1f, 100.0f);
    expectedProjection[1][1] *= -1; // Flip Y-axis for Vulkan
    if (!isEqual(camera.GetProjectionMatrix(), expectedProjection)) {
        throw std::runtime_error("Initial projection matrix is incorrect");
    }
}

void TestMovement(Camera& camera) {
    glm::vec3 initialPos = camera.GetPosition();
    camera.MoveForward(10.0f);
    if (camera.GetPosition() != initialPos + glm::vec3(0.0f, 0.0f, -1.0f)) {
        throw std::runtime_error("MoveForward failed");
    }

    initialPos = camera.GetPosition();
    camera.MoveRight(1.0f);
    if (camera.GetPosition() != initialPos + glm::vec3(1.0f, 0.0f, 0.0f)) {
        throw std::runtime_error("MoveRight failed");
    }

    initialPos = camera.GetPosition();
    camera.MoveUp(1.0f);
    if (camera.GetPosition() != initialPos + glm::vec3(0.0f, 1.0f, 0.0f)) {
        throw std::runtime_error("MoveUp failed");
    }
}

void TestRotation(Camera& camera) {
    camera.Rotate(90.0f, 0.0f); // Rotate 90 degrees around Y-axis
    glm::vec3 front = glm::normalize(glm::vec3(camera.GetViewMatrix()[2]));
    if (std::abs(front.x + 1.0f) > 1e-6f || std::abs(front.y) > 1e-6f || std::abs(front.z) > 1e-6f) {
        throw std::runtime_error("Rotation failed");
    }
}

void TestSetPosition(Camera& camera) {
    glm::vec3 newPos(5.0f, -3.0f, 2.0f);
    camera.SetPosition(newPos);
    if (camera.GetPosition() != newPos) {
        throw std::runtime_error("SetPosition failed");
    }
}

void TestSetTarget(Camera& camera) {
    camera.SetPosition(glm::vec3(0.0f, 0.0f, 5.0f));
    camera.SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    glm::vec3 front = glm::normalize(glm::vec3(camera.GetViewMatrix()[2]));
    if (std::abs(front.x) > 1e-6f || std::abs(front.y) > 1e-6f || std::abs(front.z + 1.0f) > 1e-6f) {
        throw std::runtime_error("SetTarget failed");
    }
}

void TestSetAspectRatio(Camera& camera) {
    float newAspect = 16.0f / 9.0f;
    camera.SetAspectRatio(newAspect);

    glm::mat4 expected = glm::perspective(glm::radians(45.0f), newAspect, 0.1f, 100.0f);
    expected[1][1] *= -1; // Flip Y-axis for Vulkan
    if (!isEqual(camera.GetProjectionMatrix(), expected)) {
        throw std::runtime_error("SetAspectRatio failed");
    }
}

TestResult RunCameraTest(const std::string& testName, std::function<void(Camera&)> testFunction) {
    Camera camera(45.0f, 1.778f, 0.1f, 100.0f);
    try {
        testFunction(camera);
        return { testName, true, "" };
    } catch (const std::exception& e) {
        return { testName, false, e.what() };
    } catch (...) {
        return { testName, false, "Unknown exception occurred" };
    }
}

int main() {
    std::vector<TestResult> testResults;
    testResults.push_back(RunCameraTest("Initial State", TestInitialState));
    testResults.push_back(RunCameraTest("Movement", TestMovement));
    testResults.push_back(RunCameraTest("Rotation", TestRotation));
    testResults.push_back(RunCameraTest("Set Position", TestSetPosition));
    testResults.push_back(RunCameraTest("Set Target", TestSetTarget));
    testResults.push_back(RunCameraTest("Set Aspect Ratio", TestSetAspectRatio));

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
        spdlog::info("All Camera Tests Passed!");
        return 0;
    } else {
        spdlog::error("Some Camera Tests Failed.");
        return 1;
    }
}