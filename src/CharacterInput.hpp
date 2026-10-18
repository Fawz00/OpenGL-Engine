#pragma once

#include <GLFW/glfw3.h>
#include <array>

#include "I_InputDevice.hpp"
#include "EventBus.hpp"
#include "SystemEvents.hpp"

class TextInputContext; // forward declaration

class CharacterInput : I_InputDevice {
	public:
		~CharacterInput();

		void setInputContext(TextInputContext* context) { textInputContext = context; }
		TextInputContext* getTextInputContext() const { return textInputContext; }

	private:
		void handleChar(unsigned int codepoint);

		EventBus::ListenerId charListenerID = 0;

		TextInputContext* textInputContext = nullptr;

		void init() override;
		void update() override;
		void destroy() override;
		friend class Input;
};