#include <GLFW/glfw3.h>

int main() {
    // Create a window
    if (!glfwInit()) {
        return -1;
    }

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(640, 480, "Hello World", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    
    GLFWwindow* fakeWindow = nullptr;


    // Make the window's context current
    glfwMakeContextCurrent(fakeWindow);

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(fakeWindow)) {
        // Swap front and back buffers
        glfwSwapBuffers(fakeWindow);

        // Poll for and process events
        glfwPollEvents();
    }

    // Terminate GLFW
    glfwTerminate();

    // Exit the program
    return 0;
}
