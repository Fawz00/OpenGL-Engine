#include "ImGuiWindow.hpp"

#include "Window.hpp"
#include <GLFW/glfw3.h>

#include "Time.hpp"
#include "FpsTool.hpp"

GLFWwindow* ImGuiWindow::g_Window = nullptr;
std::array<GLFWcursor*, ImGuiMouseCursor_COUNT> ImGuiWindow::myCursors = { nullptr };
std::vector<std::string>* ImGuiWindow::logMessages;

void ImGuiWindow::init(GLFWwindow* window) {
	g_Window = window;

	myCursors[ImGuiMouseCursor_Arrow] = Window::cursorArrow;
	myCursors[ImGuiMouseCursor_TextInput] = Window::cursorText;
	myCursors[ImGuiMouseCursor_Hand] = Window::cursorHand;
	myCursors[ImGuiMouseCursor_ResizeAll] = Window::cursorMove;
	myCursors[ImGuiMouseCursor_ResizeEW] = Window::cursorResizeEW;
	myCursors[ImGuiMouseCursor_ResizeNS] = Window::cursorResizeNS;
	myCursors[ImGuiMouseCursor_ResizeNWSE] = Window::cursorResizeNWSE;
	myCursors[ImGuiMouseCursor_ResizeNESW] = Window::cursorResizeNESW;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange; // allow ImGui to request cursors
	//io.MouseDrawCursor = true;

	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");

	// Logging
	logWatcherId = EventBus::subscribe<DebugLogEvent>([](const DebugLogEvent& e) {
		handleLog(e.timestamp, e.message, e.level);
	});
}
void ImGuiWindow::handleLog(std::chrono::system_clock::time_point timestamp, const std::string& msg, Debug::LogLevel lv) {
	if (logMessages) {
		logMessages->push_back(msg);
		if (logMessages->size() > 20) {
			logMessages->erase(logMessages->begin());
		}
	}
}

void ImGuiWindow::beginFrame() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImGuiWindow::endFrame() {
	// Rendering
	updateMouseCursor();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiWindow::shutdown() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	g_Window = nullptr;

	// Logging
	if (logWatcherId != 0) {
		EventBus::unsubscribe<DebugLogEvent>(logWatcherId);
		logWatcherId = 0;
	}
	if (logMessages) {
		delete logMessages;
		logMessages = nullptr;
	}
}

void ImGuiWindow::updateMouseCursor() {
	ImGuiIO& io = ImGui::GetIO();
	GLFWwindow* win = g_Window;
	ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();

	if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
		return;
	
	if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None) {
		//glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}
	else
	{
		// Use your cursor array from init()
		glfwSetCursor(win, myCursors[imgui_cursor] ? myCursors[imgui_cursor] : Window::cursorArrow);
		//glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}



void ImGuiWindow::drawWindow() {
	// dummy entity list for next development
	struct Entity {
		std::string name;
	};
	std::vector<Entity> entities = {
		{"Entity 1"},
		{"Entity 2"},
		{"Entity 3"},
		{"Entity 4"},
		{"Entity 5"},
	};
	int selectedEntity = 0;

	ImGui::Begin("Scene Hierarchy");
	ImGui::Text("Entities:");
	for (int i = 0; i < entities.size(); i++) {
		if (ImGui::Selectable(entities[i].name.c_str(), selectedEntity == i)) {
			selectedEntity = i;
		}
	}
	ImGui::End();

	// Show statistics window
	ImGui::Begin("Statistics");
	ImGui::Text("FPS: %.1f", FpsTool::getFps());
	ImGui::Text("Frame Time: %.3f ms", Time::getLastDeltaTime() * 1000.0f);
	ImGui::Text("Current Time: %.2f s", Time::getTime());
	ImGui::Text("Frame count: %d", ImGui::GetFrameCount());
	ImGui::End();

	// Show log from EventBus
	ImGui::Begin("Event Log");
	if (!logMessages) {
		logMessages = new std::vector<std::string>();
	}
	for (const auto& msg : *logMessages) {
		ImGui::TextUnformatted(msg.c_str());
	}
	ImGui::End();
}
