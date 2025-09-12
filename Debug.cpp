#include "Debug.hpp"
#include "EventBus.hpp"

void Debug::log(const std::string& message) {
	EventBus::publish(DebugLogEvent(message, Constants::LogLevel::Info));
	std::cout << "[LOG] " << message << std::endl;
}
void Debug::logWarn(const std::string& message) {
	EventBus::publish(DebugLogEvent(message, Constants::LogLevel::Warning));
	std::cout << "[WARN] " << message << std::endl;
}
void Debug::logError(const std::string& message) {
	EventBus::publish(DebugLogEvent(message, Constants::LogLevel::Error));
	std::cerr << "[ERROR] " << message << std::endl;
}