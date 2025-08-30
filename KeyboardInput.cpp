#include "KeyboardInput.h"

void KeyboardInput::init() {
    keyDown.fill(false);
    keyPressed.fill(false);
    keyHold.fill(false);
    keyReleased.fill(false);
}

KeyboardInput::~KeyboardInput() {
    // nothing to free, GLFW handles callback automatically
}

void KeyboardInput::attachToWindow(GLFWwindow* window) {
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, KeyboardInput::keyCallback);
}

void KeyboardInput::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* self = static_cast<KeyboardInput*>(glfwGetWindowUserPointer(window));
    if (self) {
        self->handleKey(key, scancode, action, mods);
    }
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
        keyHold[key] = true;
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
