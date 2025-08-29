#include<iostream>

class Debug {
	public:
		static void log(const std::string& message) {
			std::cout << "[LOG] " << message << std::endl;
		}
		static void logWarn(const std::string& message) {
			std::cout << "[WARN] " << message << std::endl;
		}
		static void logError(const std::string& message) {
			std::cerr << "[ERROR] " << message << std::endl;
		}
};