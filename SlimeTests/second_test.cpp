#include <iostream>
#include "Camera.h"
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