#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "Window.cpp"

int main()
{
    Window window(1280, 720, "OpenGL Engine [Native]");
    window.create(/*maximized*/ false, /*vsync*/ true);
    if (!window.isInitialized()) return -1;

    // Try to load custom cursor
    GLFWcursor* cursor = window.loadCursor("Resources/engine/textures/cursor.png", 0, 0);
    if (cursor) {
        glfwSetCursor(window.getGLFWwindow(), cursor);
    }
    else {
		Debug::logWarn("Using default system cursor.");
    }

    // Main loop
    while (!window.shouldClose()) {
        // Call glViewport once per resize:
        if (window.resized()) {
            glViewport(0, 0, window.width(), window.height());
            window.setResized(false);
        }

        // Example clear
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Poll+swap done inside window.update()
        window.update();
    }

    window.destroy();
    return 0;
}