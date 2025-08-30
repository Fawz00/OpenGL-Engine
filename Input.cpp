#include "Input.h"

void Input::attachToWindow(GLFWwindow* window) {
	keyboard.attachToWindow(window);
	mouse.attachToWindow(window);
}

void Input::update() {
	keyboard.update();
	mouse.update();
}
