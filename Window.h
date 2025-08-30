#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

#include "stb_image.h"
#include "Debug.h"
#include "EventBus.h"
#include "SystemEvents.h"

class Window {
public:
    // Static interface
    static void create(int width, int height, const std::string& title, bool maximized = false, bool vsync = true);
    static void destroy();
    static GLFWwindow* getGLFWwindow();
    static bool shouldClose();
    static void setShouldClose(bool v);
    static bool focused();
    static bool resized();
    static void setResized(bool b);
    static int width();
    static int height();
    static bool isVsync();
    static void toggleFullscreen();
    static void poolEvent();
    static void drawFrame();
    static GLFWcursor* loadCursor(const char* path, int hotspotX, int hotspotY);

private:
    // Static state
    static inline GLFWwindow* window = nullptr;
    static inline GLFWcursor* cursor = nullptr;

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

    // Static callbacks
    static void s_window_size_cb(GLFWwindow* win, int w, int h);
    static void s_window_focus_cb(GLFWwindow* win, int focused);
	static void s_key_cb(GLFWwindow* win, int key, int scancode, int action, int mods);
	static void s_char_cb(GLFWwindow* win, unsigned int codepoint);
	static void s_cursor_pos_cb(GLFWwindow* win, double xpos, double ypos);
	static void s_mouse_button_cb(GLFWwindow* win, int button, int action, int mods);
	static void s_scroll_cb(GLFWwindow* win, double offsetx, double offsety);
};
