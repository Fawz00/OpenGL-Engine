#pragma once

#include <string>
#include <ctime>
#include <glm/glm.hpp>

class Actor {
public:
	Actor(std::string);
	Actor(char*);
	virtual ~Actor() = default;

	// No copy
	Actor(const Actor&) = delete;
	Actor& operator=(const Actor&) = delete;

	// Move allowed
	Actor(Actor&&) noexcept = default;
	Actor& operator=(Actor&&) noexcept = default;

	void setPosition(glm::vec3 newPosition);
	void setPosition(float x, float y, float z);
	glm::vec3 getPosition();
	std::string getFilePath();


protected:
	std::string ID;
	bool isActive = true;

	std::string filePath;
	glm::vec3 location;
};