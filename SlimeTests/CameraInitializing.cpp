#include <iostream>
#include <stdexcept>
#include "Camera.h"
#include <spdlog/spdlog.h>
#include <glm/vec3.hpp>

void CameraInit() {
    Camera camera = Camera(90.0f, 800.0f / 600.0f, 0.001f, 100.0f);

    glm::vec3 testPos = glm::vec3(90, 90, 90);
    camera.SetPosition(testPos);

    if (camera.GetPosition() != glm::vec3(90, 90, 90)) {
        throw std::runtime_error("Camera position test failed");
    }
}

int main() {
    try {
        CameraInit();
    }
    catch (const std::exception& e) {
        spdlog::error("Test failed with exception: {}", e.what());
        return 1;
    }
    catch (...) {
        spdlog::error("Unknown exception occurred during testing.");
        return 2;
    }
    spdlog::info("Camera Initializing Test Passed!");
    return 0;
}
