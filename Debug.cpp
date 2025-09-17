#include "Debug.hpp"

#include <cstdarg>   // va_list, va_start, va_end
#include <cstdio>    // vsnprintf
#include <vector>

#include "EventBus.hpp"
#include "SystemEvents.hpp"

namespace {
	// Helper function to format strings
    std::string formatString(const char* fmt, va_list args) {
        va_list args_copy;
        va_copy(args_copy, args);

		// Get the size needed
        int size = std::vsnprintf(nullptr, 0, fmt, args_copy);
        va_end(args_copy);

        if (size <= 0) return "";

        std::vector<char> buffer(size + 1);
        std::vsnprintf(buffer.data(), buffer.size(), fmt, args);
        return std::string(buffer.data(), size);
    }
}

void Debug::log(const std::string& msg) {
    EventBus::publish(DebugLogEvent(msg, Constants::LogLevel::Warning));
    std::cout << "[LOG] " << msg << std::endl;
}

void Debug::log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::string message = formatString(fmt, args);
    va_end(args);

    EventBus::publish(DebugLogEvent(message, Constants::LogLevel::Info));
    std::cout << "[LOG] " << message << std::endl;
}

void Debug::logWarn(const std::string& msg) {
    EventBus::publish(DebugLogEvent(msg, Constants::LogLevel::Warning));
    std::cout << "[WARN] " << msg << std::endl;
}

void Debug::logWarn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::string message = formatString(fmt, args);
    va_end(args);

    EventBus::publish(DebugLogEvent(message, Constants::LogLevel::Warning));
    std::cout << "[WARN] " << message << std::endl;
}

void Debug::logError(const std::string& msg) {
    EventBus::publish(DebugLogEvent(msg, Constants::LogLevel::Error));
    std::cerr << "[ERROR] " << msg << std::endl;
}

void Debug::logError(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::string message = formatString(fmt, args);
    va_end(args);

    EventBus::publish(DebugLogEvent(message, Constants::LogLevel::Error));
    std::cerr << "[ERROR] " << message << std::endl;
}
