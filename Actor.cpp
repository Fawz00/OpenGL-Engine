#include "Actor.hpp"

Actor::Actor(std::string path)
	: filePath(std::move(path)), location(0.0f, 0.0f, 0.0f)
{
	// Generate a simple unique ID based on current time and address
	ID = "actor_" + std::to_string(reinterpret_cast<uintptr_t>(this)) + "_" + std::to_string(static_cast<uint64_t>(time(nullptr)));
}

Actor::Actor(char* path)
	: Actor(std::string(path))
{
	// Nothing else needed here
}

void Actor::setPosition(glm::vec3 newPosition) {
	location = newPosition;
}

void Actor::setPosition(float x, float y, float z) {
	location = glm::vec3(x, y, z);
}

glm::vec3 Actor::getPosition() {
	return location;
}

std::string Actor::getFilePath() {
	return filePath;
}
