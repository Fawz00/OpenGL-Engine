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

	Window::create(1280, 720, "OpenGL Engine [Native]", false, false);
    if (glfwInit() == GLFW_FALSE) return -1;

    // Try to load custom cursor
    GLFWcursor* cursor = Window::loadCursor("Resources/engine/textures/cursor.png", 0, 1);
    if (cursor) {
        glfwSetCursor(Window::getGLFWwindow(), cursor);
    }
    else {
		Debug::logWarn("Using default system cursor.");
    }

    // Attach input handlers
	Input::init();

	// Initialize renderer
	EngineRenderer::onInit();

    // Main loop
    while (!Window::shouldClose()) {
		Window::poolEvent();

        // Call glViewport once per resize:
        if (Window::resized()) {
            glViewport(0, 0, Window::width(), Window::height());
            Window::setResized(false);
        }

        if (Input::keyboard.isKeyPressed(GLFW_KEY_F11)) {
            Window::toggleFullscreen();
		}
        if (Input::keyboard.isKeyPressed(GLFW_KEY_ESCAPE)) {
            Window::setShouldClose(true);
		}
        if (Input::mouse.isButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            double mx = Input::mouse.getMouseX();
			double my = Input::mouse.getMouseY();
			Debug::log("Mouse Click at: " + std::to_string(mx) + ", " + std::to_string(my));
		}

        // Example clear
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		EngineRenderer::onUpdate(AppTime::getCurrentDeltaTime());

        Window::drawFrame();
        Input::update();
        AppTime::update();
        FpsTool::endFrame();
    }

	Input::destroy();
    Window::destroy();
    return 0;
}