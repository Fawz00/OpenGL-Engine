#include "Debug.h"

void Debug::log(const std::string& message) {
	std::cout << "[LOG] " << message << std::endl;
}
void Debug::logWarn(const std::string& message) {
	std::cout << "[WARN] " << message << std::endl;
}
void Debug::logError(const std::string& message) {
	std::cerr << "[ERROR] " << message << std::endl;
}