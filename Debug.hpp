#pragma once

#include <iostream>
#include <string>

namespace Debug {
	enum LogLevel {
		Info,
		Warning,
		Error,
		Debug
	};

	void log(const std::string& msg);
	void log(const char* fmt, ...);
	void logWarn(const std::string& msg);
	void logWarn(const char* fmt, ...);
	void logError(const std::string& msg);
	void logError(const char* fmt, ...);
};