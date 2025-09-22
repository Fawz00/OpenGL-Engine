#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <array>
#include <vector>
#include <chrono>

#include "EventBus.hpp"

typedef struct GLFWwindow GLFWwindow;
typedef struct GLFWcursor;

class ImGuiWindow {
public:
	static void drawWindow();

private:
	inline static char buf[128] = "";
	static GLFWwindow* g_Window;

	static std::array<GLFWcursor*, ImGuiMouseCursor_COUNT> myCursors;

	static void init(GLFWwindow* window);
	static void beginFrame();
	static void updateMouseCursor();
	static void endFrame();
	static void shutdown();

	// For logging
	static std::vector<std::string>* logMessages;
	static inline EventBus::ListenerId logWatcherId = 0;
	static void handleLog(std::chrono::system_clock::time_point timestamp, const std::string& msg, Debug::LogLevel lv);

	friend int main();
};