#include "KeyboardInput.h"
#include "Debug.h"

void KeyboardInput::init() {
    keyDown.fill(false);
    keyPressed.fill(false);
    keyHold.fill(false);
    keyReleased.fill(false);

    keyDownListenerID = EventBus::subscribe<KeyEvent>([this](const KeyEvent& e) {
		handleKey(e.key, e.scancode, e.action, e.mods);
	});
}

void KeyboardInput::destroy() {
    EventBus::unsubscribe<KeyEvent>(keyDownListenerID);
}

KeyboardInput::~KeyboardInput() {
}

void KeyboardInput::handleKey(int key, int scancode, int action, int mods) {
    if (key < 0 || key >= GLFW_KEY_LAST) return;

    rawKey = key;
    rawScancode = scancode;
    rawAction = action;
    rawMods = mods;

    ctrlDown = (mods & GLFW_MOD_CONTROL) != 0;
    shiftDown = (mods & GLFW_MOD_SHIFT) != 0;
    altDown = (mods & GLFW_MOD_ALT) != 0;

    if (action == GLFW_PRESS) {
        keyDown[key] = true;
        keyReleased[key] = false;
        keyHold[key] = false;
        keyPressed[key] = true;
    }
    else if (action == GLFW_RELEASE) {
        keyDown[key] = false;
        keyPressed[key] = false;
        keyHold[key] = false;
        keyReleased[key] = true;
    }
    else if (action == GLFW_REPEAT) {
        keyDown[key] = true;
        keyReleased[key] = false;
        keyPressed[key] = false;
        keyHold[key] = true;
    }

    if (textInputContext) {
        //textInputContext->handleKey(*this);
    }
}

void KeyboardInput::update() {
    for (int i = 0; i < GLFW_KEY_LAST; i++) {
        keyPressed[i] = false;
        keyReleased[i] = false;
    }
}

bool KeyboardInput::isKeyDown(int key) const {
    return keyDown[key];
}

bool KeyboardInput::isKeyPressed(int key) const {
    return keyPressed[key];
}

bool KeyboardInput::isKeyHeld(int key) const {
    return keyHold[key];
}

bool KeyboardInput::isKeyReleased(int key) const {
    return keyReleased[key];
}

std::array<int, 4> KeyboardInput::getRawInput() const {
    return { rawKey, rawScancode, rawAction, rawMods };
}
