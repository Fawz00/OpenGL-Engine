#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "Window.h"
#include "Debug.h"
#include "EngineRenderer.h"
#include "FpsTool.h"
#include "AppTime.h"
#include "Input.h"

int main()
{
    std::cout << __cplusplus << "\n";

	Window::create(1280, 720, "OpenGL Engine [Native]", false, true);
    if (!glfwInit()) return -1;

    // Try to load custom cursor
    GLFWcursor* cursor = Window::loadCursor("Resources/engine/textures/cursor.png", 0, 0);
    if (cursor) {
        glfwSetCursor(Window::getGLFWwindow(), cursor);
    }
    else {
		Debug::logWarn("Using default system cursor.");
    }

    // Attach input handlers
    Input::attachToWindow(Window::getGLFWwindow());

	// Initialize renderer
	EngineRenderer::onInit();

    // Main loop
    while (!Window::shouldClose()) {
		Window::poolEvent();
        Input::update();

        // Call glViewport once per resize:
        if (Window::resized()) {
            glViewport(0, 0, Window::width(), Window::height());
            Window::setResized(false);
        }

        // Example clear
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		EngineRenderer::onUpdate(AppTime::getCurrentDeltaTime());

        Window::drawFrame();
        AppTime::update();
        FpsTool::endFrame();
    }

    Window::destroy();
    return 0;
}