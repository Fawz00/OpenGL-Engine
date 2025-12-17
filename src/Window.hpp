#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>

#include "stb_image.h"

#include "Debug.hpp"
#include "EventBus.hpp"
#include "SystemEvents.hpp"

class Window {
public:
    // Static interface
    static void create(int width, int height, const std::string& title, bool maximized = false, bool vsync = true);
	static void setupCursors();
    static void destroy();
    static GLFWwindow* getGLFWwindow();
    static bool shouldClose();
    static void setShouldClose(bool v);
    static bool focused();
    static bool resized();
    static int width();
    static int height();
	static void enableVsync(bool v);
    static bool isVsync();
    static void toggleFullscreen();
    static void poolEvent();
    static void drawFrame();
    static void loadCursor(GLFWcursor*& cursor, const char* path, int hotspotX, int hotspotY);
	static void showCursor(bool show);
	static bool isCursorVisible();

    static inline GLFWcursor* cursorArrow = nullptr;
    static inline GLFWcursor* cursorHand = nullptr;
    static inline GLFWcursor* cursorText = nullptr;
    static inline GLFWcursor* cursorResizeEW = nullptr;
    static inline GLFWcursor* cursorResizeNS = nullptr;
    static inline GLFWcursor* cursorResizeNWSE = nullptr;
    static inline GLFWcursor* cursorResizeNESW = nullptr;
    static inline GLFWcursor* cursorMove = nullptr;

private:
    // Static state
    static inline GLFWwindow* window = nullptr;

    static inline int WINDOW_WIDTH = 800;
    static inline int WINDOW_HEIGHT = 600;
	static inline int WINDOW_LAST_POS_X = 0;
	static inline int WINDOW_LAST_POS_Y = 0;
	static inline int WINDOW_LAST_WIDTH = 800;
	static inline int WINDOW_LAST_HEIGHT = 600;
	static inline std::string WINDOW_TITLE = "OpenGL Window";

    static inline bool isResized = false;
    static inline bool isFocused = true;
    static inline bool isFullscreen = false;
    static inline bool vsync = true;
	static inline bool cursorVisible = true;

    static void setResized(bool b);
	friend int main();

    // Static callbacks
    static void s_window_size_cb(GLFWwindow* win, int w, int h);
    static void s_window_focus_cb(GLFWwindow* win, int focused);
	static void s_key_cb(GLFWwindow* win, int key, int scancode, int action, int mods);
	static void s_char_cb(GLFWwindow* win, unsigned int codepoint);
	static void s_cursor_pos_cb(GLFWwindow* win, double xpos, double ypos);
	static void s_mouse_button_cb(GLFWwindow* win, int button, int action, int mods);
	static void s_scroll_cb(GLFWwindow* win, double offsetx, double offsety);
};
