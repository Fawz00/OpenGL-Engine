#include "Input.h"

void Input::init() {
	keyboard.init();
	mouse.init();
}

void Input::update() {
	keyboard.update();
	mouse.update();
}

void Input::destroy() {
	keyboard.destroy();
	mouse.destroy();
}
