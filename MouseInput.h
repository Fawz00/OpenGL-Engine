#pragma once

#include <GLFW/glfw3.h>
#include <array>

class MouseInput {
public:
    void init();
    ~MouseInput();

    void attachToWindow(GLFWwindow* window);

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

    void update(); // call after glfwPollEvents()
	friend class Input;
};
