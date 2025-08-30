#pragma once

#include <cstring>
#include <GLFW/glfw3.h>
#include <array>
#include <memory>

#include "EventBus.h"
#include "SystemEvents.h"

class TextInputContext; // forward declaration

class KeyboardInput {
public:
    ~KeyboardInput();

    bool isKeyDown(int key) const;
    bool isKeyPressed(int key) const;
    bool isKeyHeld(int key) const;
    bool isKeyReleased(int key) const;

    std::array<int, 4> getRawInput() const;

    bool isCtrlDown() const { return ctrlDown; }
    bool isShiftDown() const { return shiftDown; }
    bool isAltDown() const { return altDown; }

    void setInputContext(TextInputContext* context) { textInputContext = context; }
    TextInputContext* getTextInputContext() const { return textInputContext; }

private:
    void handleKey(int key, int scancode, int action, int mods);

	EventBus::ListenerId keyDownListenerID = 0;

    std::array<bool, GLFW_KEY_LAST> keyDown{};
    std::array<bool, GLFW_KEY_LAST> keyPressed{};
    std::array<bool, GLFW_KEY_LAST> keyHold{};
    std::array<bool, GLFW_KEY_LAST> keyReleased{};

    bool ctrlDown = false;
    bool shiftDown = false;
    bool altDown = false;

    int rawKey = -1;
    int rawScancode = -1;
    int rawAction = -1;
    int rawMods = -1;

    TextInputContext* textInputContext = nullptr;

    void init();
    void update(); // call after glfwPollEvents()
    void destroy();
	friend class Input;
};
