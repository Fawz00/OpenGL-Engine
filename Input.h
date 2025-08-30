#pragma once
#include "KeyboardInput.h"
#include "MouseInput.h"

class Input {
	public:
		static void attachToWindow(GLFWwindow* window);

	private:
		inline static KeyboardInput keyboard = KeyboardInput();
		inline static MouseInput mouse = MouseInput();

		static void update();
		friend int main();
};