#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <array>

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

	friend int main();
};