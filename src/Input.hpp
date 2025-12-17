#pragma once

#include "KeyboardInput.hpp"
#include "MouseInput.hpp"

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