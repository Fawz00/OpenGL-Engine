#pragma once

#include "KeyboardInput.hpp"
#include "MouseInput.hpp"
#include "CharacterInput.hpp"

class Input {
	public:
		inline static KeyboardInput keyboard = KeyboardInput();
		inline static MouseInput mouse = MouseInput();
		inline static CharacterInput character = CharacterInput();

	private:
		static void init();
		static void update();
		static void destroy();
		friend int main();
};