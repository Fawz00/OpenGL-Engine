#include "CharacterInput.hpp"

void CharacterInput::init() {
	charListenerID = EventBus::subscribe<CharEvent>([this](const CharEvent& e) {
		handleChar(e.codepoint);
		});
}

void CharacterInput::destroy() {
	EventBus::unsubscribe<CharEvent>(charListenerID);
}

void CharacterInput::update() {
	// No update needed for character input
}

CharacterInput::~CharacterInput() {
}

void CharacterInput::handleChar(unsigned int codepoint)
{
}