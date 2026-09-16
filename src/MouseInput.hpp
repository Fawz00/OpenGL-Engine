#pragma once

#include <GLFW/glfw3.h>
#include <array>

#include "I_InputDevice.hpp"
#include "EventBus.hpp"
#include "SystemEvents.hpp"

class MouseInput : I_InputDevice {
public:
    ~MouseInput();

    bool isButtonDown(int button) const;
    bool isButtonPressed(int button) const;
    bool isButtonReleased(int button) const;

    double getMouseX() const { return mouseX; }
    double getMouseY() const { return mouseY; }

    double getScrollX() const { return scrollX; }
    double getScrollY() const { return scrollY; }

private:
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double offsetx, double offsety);

	EventBus::ListenerId cursorPosListenerID = 0;
    EventBus::ListenerId mouseButtonListenerID = 0;
    EventBus::ListenerId scrollListenerID = 0;

    void handleCursorPos(double xpos, double ypos);
    void handleMouseButton(int button, int action, int mods);
    void handleScroll(double offsetx, double offsety);

    std::array<bool, GLFW_MOUSE_BUTTON_LAST> buttonDown{};
    std::array<bool, GLFW_MOUSE_BUTTON_LAST> buttonPressed{};
    std::array<bool, GLFW_MOUSE_BUTTON_LAST> buttonReleased{};

    double mouseX = 0.0;
    double mouseY = 0.0;

    double scrollX = 0.0;
    double scrollY = 0.0;

    void init() override;
    void update() override; // call after glfwPollEvents()
	void destroy() override;
	friend class Input;
};
