#include<iostream>

#define STB_IMAGE_IMPLEMENTATION

// Note: Don't change the order of these includes (from some library dependencies)
#include "ImGuiWindow.hpp"
#include "Window.hpp"
#include "Debug.hpp"
#include "EngineRenderer.hpp"
#include "FpsTool.hpp"
#include "Time.hpp"
#include "Input.hpp"

int main()
{
    std::cout << __cplusplus << "\n";

	Window::create(1280, 720, "OpenGL 3D", false, false);
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
			Window::showCursor(!Window::isCursorVisible());
			if (Window::isCursorVisible())
			{
				Time::timeScale = 0.2f;
			}
			else
			{
				Time::timeScale = 1.0f;
			}
		}
		if (Input::keyboard.isKeyPressed(GLFW_KEY_DELETE)) {
			Window::setShouldClose(true);
		}

		// Start the ImGui frame
        ImGuiWindow::beginFrame();

		ImGuiWindow::drawWindow();
		EngineRenderer::onUpdate();

		// End the ImGui
		ImGuiWindow::endFrame();

        Window::drawFrame();
        Input::update();
        Time::update();
        FpsTool::endFrame();
    }

	EngineRenderer::onDestroy();

	ImGuiWindow::shutdown();
	Input::destroy();
    Window::destroy();

	/*
		Return code on exit :
		- 0x00000000 : Normal.
		- 0xffffffff : Failed to create window.
		- 0xc0000409 : Stack buffer overrun
			Probably due to some destructors being called in the wrong order on exit.
			Possibly related to static variables in multiple translation units.
			Or related to shader errors not being properly handled.
	*/

    return 0;
}