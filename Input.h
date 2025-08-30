#pragma once
#include "KeyboardInput.h"
#include "MouseInput.h"

class Input {
	public:
		inline static KeyboardInput keyboard = KeyboardInput();
		inline static MouseInput mouse = MouseInput();

	private:
		static void init();
		static void update();
		static void destroy();
		friend int main();
};