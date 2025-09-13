#include "ImGuiWindow.hpp"

#include "Window.hpp"
#include <GLFW/glfw3.h>

GLFWwindow* ImGuiWindow::g_Window = nullptr;
std::array<GLFWcursor*, ImGuiMouseCursor_COUNT> ImGuiWindow::myCursors = { nullptr };

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

	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");
}

void ImGuiWindow::beginFrame() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImGuiWindow::endFrame() {
	// Rendering
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiWindow::shutdown() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	g_Window = nullptr;
}

void ImGuiWindow::updateMouseCursor() {
	// Handle ImGui requested cursor change
	ImGuiIO& io = ImGui::GetIO();
	GLFWwindow* win = g_Window;
	ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
	if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
		return;
	if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None) {
		glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}
	else
	{
		// Use your cursor array from init()
		glfwSetCursor(win, myCursors[imgui_cursor] ? myCursors[imgui_cursor] : Window::cursorArrow);
		glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}



void ImGuiWindow::drawWindow() {
	// dummy entity list
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

	updateMouseCursor();
}
