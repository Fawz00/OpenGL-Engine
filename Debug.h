#pragma once

#include <iostream>
#include <string>

class Debug {
	public:
		static void log(const std::string& message);
		static void logWarn(const std::string& message);
		static void logError(const std::string& message);
};