#include "MouseInput.h"

void MouseInput::init() {
    buttonDown.fill(false);
    buttonPressed.fill(false);
    buttonReleased.fill(false);
}

MouseInput::~MouseInput() {
    // GLFW handles cleanup for callbacks
}

void MouseInput::attachToWindow(GLFWwindow* window) {
    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, MouseInput::cursorPosCallback);
    glfwSetMouseButtonCallback(window, MouseInput::mouseButtonCallback);
    glfwSetScrollCallback(window, MouseInput::scrollCallback);
}

void MouseInput::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* self = static_cast<MouseInput*>(glfwGetWindowUserPointer(window));
    if (self) {
        self->handleCursorPos(xpos, ypos);
    }
}

void MouseInput::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto* self = static_cast<MouseInput*>(glfwGetWindowUserPointer(window));
    if (self) {
        self->handleMouseButton(button, action, mods);
    }
}

void MouseInput::scrollCallback(GLFWwindow* window, double offsetx, double offsety) {
    auto* self = static_cast<MouseInput*>(glfwGetWindowUserPointer(window));
    if (self) {
        self->handleScroll(offsetx, offsety);
    }
}

void MouseInput::handleCursorPos(double xpos, double ypos) {
    mouseX = xpos;
    mouseY = ypos;
}

void MouseInput::handleMouseButton(int button, int action, int mods) {
    if (button < 0 || button >= GLFW_MOUSE_BUTTON_LAST) return;

    if (action == GLFW_PRESS) {
        if (!buttonDown[button]) {
            buttonPressed[button] = true; // cuma sekali per frame
        }
        buttonDown[button] = true;
    }
    else if (action == GLFW_RELEASE) {
        buttonDown[button] = false;
        buttonReleased[button] = true;
    }
}

void MouseInput::handleScroll(double offsetx, double offsety) {
    scrollX += offsetx;
    scrollY += offsety;
}

void MouseInput::update() {
    for (int i = 0; i < GLFW_MOUSE_BUTTON_LAST; i++) {
        buttonPressed[i] = false;
        buttonReleased[i] = false;
    }
}

bool MouseInput::isButtonDown(int button) const {
    return buttonDown[button];
}

bool MouseInput::isButtonPressed(int button) const {
    return buttonPressed[button];
}

bool MouseInput::isButtonReleased(int button) const {
    return buttonReleased[button];
}
