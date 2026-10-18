#include "Input.hpp"

void Input::init() {
	keyboard.init();
	mouse.init();
	character.init();
}

void Input::update() {
	keyboard.update();
	mouse.update();
	character.update();
}

void Input::destroy() {
	keyboard.destroy();
	mouse.destroy();
	character.destroy();
}
