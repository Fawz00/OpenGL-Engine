#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "stb_image.h"
#include "Debug.h"
#include <string>

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
    static inline bool isResized = false;
    static inline bool isFocused = true;
    static inline bool isFullscreen = false;
    static inline bool vsync = true;

    // Static callbacks
    static void s_window_size_cb(GLFWwindow* win, int w, int h);
    static void s_window_focus_cb(GLFWwindow* win, int focused);
};
