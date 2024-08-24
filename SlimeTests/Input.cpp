//#include "InputManager.h"
//#include "ModelManager.h"
//#include "spdlog/spdlog.h"
//#include <memory>
//
//// Mock classes for GLFW dependencies
//struct GLFWwindow {};
//
//// Mock functions for GLFW
//void glfwSetKeyCallback(GLFWwindow* window, GLFWkeyfun callback) {}
//void glfwSetMouseButtonCallback(GLFWwindow* window, GLFWmousebuttonfun callback) {}
//void glfwSetScrollCallback(GLFWwindow* window, GLFWscrollfun callback) {}
//int glfwGetKey(GLFWwindow* window, int key) { return 0; }
//int glfwGetMouseButton(GLFWwindow* window, int button) { return 0; }
//void glfwGetCursorPos(GLFWwindow* window, double* xpos, double* ypos) {}
//
//void TestInputManagerCreation(ModelManager& modelManager) {
//    auto window = std::make_unique<GLFWwindow>();
//    InputManager inputManager(window.get());
//
//    // Check that the input manager was created successfully
//    if (&inputManager == nullptr) {
//        throw std::runtime_error("Failed to create InputManager");
//    }
//}
//
//void TestKeyPressAndRelease(ModelManager& modelManager) {
//    auto window = std::make_unique<GLFWwindow>();
//    InputManager inputManager(window.get());
//
//    // Simulate key press
//    inputManager.KeyCallback(window.get(), GLFW_KEY_A, 0, GLFW_PRESS, 0);
//    if (!inputManager.IsKeyPressed(GLFW_KEY_A) || !inputManager.IsKeyJustPressed(GLFW_KEY_A)) {
//        throw std::runtime_error("Key press not detected correctly");
//    }
//
//    // Simulate key release
//    inputManager.KeyCallback(window.get(), GLFW_KEY_A, 0, GLFW_RELEASE, 0);
//    if (inputManager.IsKeyPressed(GLFW_KEY_A) || !inputManager.IsKeyJustReleased(GLFW_KEY_A)) {
//        throw std::runtime_error("Key release not detected correctly");
//    }
//}
//
//void TestMouseButtonPressAndRelease(ModelManager& modelManager) {
//    auto window = std::make_unique<GLFWwindow>();
//    InputManager inputManager(window.get());
//
//    // Simulate mouse button press
//    inputManager.MouseButtonCallback(window.get(), GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0);
//    if (!inputManager.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT) || !inputManager.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
//        throw std::runtime_error("Mouse button press not detected correctly");
//    }
//
//    // Simulate mouse button release
//    inputManager.MouseButtonCallback(window.get(), GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE, 0);
//    if (inputManager.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT) || !inputManager.IsMouseButtonJustReleased(GLFW_MOUSE_BUTTON_LEFT)) {
//        throw std::runtime_error("Mouse button release not detected correctly");
//    }
//}
//
//void TestMouseMovement(ModelManager& modelManager) {
//    auto window = std::make_unique<GLFWwindow>();
//    InputManager inputManager(window.get());
//
//    // Simulate mouse movement
//    inputManager.Update(); // This would normally be called by the game loop
//    auto [deltaX, deltaY] = inputManager.GetMouseDelta();
//    if (deltaX != 0 || deltaY != 0) {
//        throw std::runtime_error("Initial mouse delta should be zero");
//    }
//
//    // Simulate another mouse movement
//    inputManager.Update();
//    std::tie(deltaX, deltaY) = inputManager.GetMouseDelta();
//    // Note: We can't actually test for non-zero values here because we haven't mocked glfwGetCursorPos
//    // In a real scenario, you'd inject mock cursor positions and test the deltas
//}
//
//void TestScrollInput(ModelManager& modelManager) {
//    auto window = std::make_unique<GLFWwindow>();
//    InputManager inputManager(window.get());
//
//    // Simulate scroll
//    inputManager.ScrollCallback(window.get(), 0, 1.0);
//    if (inputManager.GetScrollDelta() != 1.0) {
//        throw std::runtime_error("Scroll input not detected correctly");
//    }
//
//    // Simulate another scroll
//    inputManager.ScrollCallback(window.get(), 0, -0.5);
//    if (inputManager.GetScrollDelta() != -0.5) {
//        throw std::runtime_error("Scroll input not updated correctly");
//    }
//}
//
//void TestMultipleKeyStates(ModelManager& modelManager) {
//    auto window = std::make_unique<GLFWwindow>();
//    InputManager inputManager(window.get());
//
//    // Press multiple keys
//    inputManager.KeyCallback(window.get(), GLFW_KEY_W, 0, GLFW_PRESS, 0);
//    inputManager.KeyCallback(window.get(), GLFW_KEY_A, 0, GLFW_PRESS, 0);
//    inputManager.KeyCallback(window.get(), GLFW_KEY_S, 0, GLFW_PRESS, 0);
//
//    if (!inputManager.IsKeyPressed(GLFW_KEY_W) || !inputManager.IsKeyPressed(GLFW_KEY_A) || !inputManager.IsKeyPressed(GLFW_KEY_S)) {
//        throw std::runtime_error("Multiple key presses not detected correctly");
//    }
//
//    // Release one key
//    inputManager.KeyCallback(window.get(), GLFW_KEY_A, 0, GLFW_RELEASE, 0);
//
//    if (!inputManager.IsKeyPressed(GLFW_KEY_W) || inputManager.IsKeyPressed(GLFW_KEY_A) || !inputManager.IsKeyPressed(GLFW_KEY_S)) {
//        throw std::runtime_error("Key states not updated correctly after release");
//    }
//}
//
//void TestMultipleMouseButtonStates(ModelManager& modelManager) {
//    auto window = std::make_unique<GLFWwindow>();
//    InputManager inputManager(window.get());
//
//    // Press multiple mouse buttons
//    inputManager.MouseButtonCallback(window.get(), GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0);
//    inputManager.MouseButtonCallback(window.get(), GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS, 0);
//
//    if (!inputManager.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT) || !inputManager.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
//        throw std::runtime_error("Multiple mouse button presses not detected correctly");
//    }
//
//    // Release one button
//    inputManager.MouseButtonCallback(window.get(), GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE, 0);
//
//    if (inputManager.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT) || !inputManager.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
//        throw std::runtime_error("Mouse button states not updated correctly after release");
//    }
//}
//
//int main() {
//    std::vector<TestResult> testResults;
//    testResults.push_back(RunTest("InputManagerCreation", TestInputManagerCreation));
//    testResults.push_back(RunTest("KeyPressAndRelease", TestKeyPressAndRelease));
//    testResults.push_back(RunTest("MouseButtonPressAndRelease", TestMouseButtonPressAndRelease));
//    testResults.push_back(RunTest("MouseMovement", TestMouseMovement));
//    testResults.push_back(RunTest("ScrollInput", TestScrollInput));
//    testResults.push_back(RunTest("MultipleKeyStates", TestMultipleKeyStates));
//    testResults.push_back(RunTest("MultipleMouseButtonStates", TestMultipleMouseButtonStates));
//
//    bool allPassed = true;
//    for (const auto& result : testResults) {
//        if (result.passed) {
//            spdlog::info("Test '{}' passed.", result.testName);
//        } else {
//            spdlog::error("Test '{}' failed with error: {}", result.testName, result.errorMessage);
//            allPassed = false;
//        }
//    }
//
//    if (allPassed) {
//        spdlog::info("All Tests Passed!");
//        return 0;
//    } else {
//        spdlog::error("Some Tests Failed.");
//        return 1;
//    }
//}