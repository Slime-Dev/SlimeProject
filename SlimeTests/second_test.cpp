#include <iostream>
#include "Camera.h"
#include <spdlog/spdlog.h>
#include <vector>

void SecondTest()
{
    Camera camera = Camera(90.0f, 800.0f / 600.0f, 0.001f, 100.0f);

    glm::vec3 testPos = glm::vec3(90, 90, 90);
    camera.SetPosition(testPos);

    assert(camera.GetPosition() == glm::vec3(90, 90, 90));
}

int main()
{
    try {
        SecondTest();
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