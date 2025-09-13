#include<iostream>

#define STB_IMAGE_IMPLEMENTATION

// Note: Don't change the order of these includes (from some library dependencies)
#include "ImGuiWindow.hpp"
#include "Window.hpp"
#include "Debug.hpp"
#include "EngineRenderer.hpp"
#include "FpsTool.hpp"
#include "AppTime.hpp"
#include "Input.hpp"

int main()
{
    std::cout << __cplusplus << "\n";
	Window::create(1280, 720, "OpenGL Engine", false, true);
	if (!Window::getGLFWwindow()) {
		Debug::logError("Failed to create window");
		return -1;
	}

	// Setup custom cursors
	Window::setupCursors();

    // Attach input handlers
	Input::init();

	// ImGUI setup
	ImGuiWindow::init(Window::getGLFWwindow());

	// Initialize renderer
	EngineRenderer::onInit();

    // Main loop
    while (!Window::shouldClose()) {
		Window::poolEvent();

        // Call glViewport once per resize:
        if (Window::resized()) {
            glViewport(0, 0, Window::width(), Window::height());
			EngineRenderer::onWindowResize();
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

		// Clear screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Start the ImGui frame
        ImGuiWindow::beginFrame();

		ImGuiWindow::drawWindow();
		EngineRenderer::onUpdate();

		// End the ImGui
		ImGuiWindow::endFrame();

        Window::drawFrame();
        Input::update();
        AppTime::update();
        FpsTool::endFrame();
    }

	EngineRenderer::onDestroy();

	ImGuiWindow::shutdown();
	Input::destroy();
    Window::destroy();
    return 0;
}