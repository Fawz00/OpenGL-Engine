#pragma once

#include <iostream>
#include <string>

#include "SystemEvents.hpp"

namespace Debug {
	void log(const std::string& message);
	void logWarn(const std::string& message);
	void logError(const std::string& message);
};