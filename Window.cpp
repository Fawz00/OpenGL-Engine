#include "Window.h"

void Window::create(int width, int height, const std::string& title, bool maximized, bool vsync_flag) {
    if (!glfwInit()) { std::cerr << "Failed to init GLFW\n"; return; }

    WINDOW_WIDTH = width;
    WINDOW_HEIGHT = height;
    vsync = vsync_flag;

    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, title.c_str(), nullptr, nullptr);
    if (!window) { glfwTerminate(); return; }

    if (!maximized) {
        const GLFWvidmode* vm = glfwGetVideoMode(glfwGetPrimaryMonitor());
        if (vm) glfwSetWindowPos(window, (vm->width - WINDOW_WIDTH) / 2, (vm->height - WINDOW_HEIGHT) / 2);
    }

    // Set static callbacks
    glfwSetWindowSizeCallback(window, &Window::s_window_size_cb);
    glfwSetWindowFocusCallback(window, &Window::s_window_focus_cb);

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cerr << "Failed to init GLAD\n"; return; }

    glfwSwapInterval(vsync ? 1 : 0);
}

void Window::destroy() {
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

GLFWwindow* Window::getGLFWwindow() { return window; }
bool Window::shouldClose() { return window ? glfwWindowShouldClose(window) : true; }
void Window::setShouldClose(bool v) { if (window) glfwSetWindowShouldClose(window, v ? GLFW_TRUE : GLFW_FALSE); }
bool Window::focused() { return isFocused; }
bool Window::resized() { return isResized; }
void Window::setResized(bool b) { isResized = b; }
int Window::width() { return WINDOW_WIDTH; }
int Window::height() { return WINDOW_HEIGHT; }
bool Window::isVsync() { return vsync; }

void Window::poolEvent() { glfwPollEvents(); }
void Window::drawFrame() { if (window) glfwSwapBuffers(window); }

void Window::toggleFullscreen() {
    if (!window) return;
    const GLFWvidmode* vm = glfwGetVideoMode(glfwGetPrimaryMonitor());
    if (!vm) return;

    if (!isFullscreen) {
        int w, h, px, py;
        glfwGetWindowSize(window, &w, &h);
        glfwGetWindowPos(window, &px, &py);

        glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, vm->width, vm->height, vm->refreshRate);
        isFullscreen = true;
    }
    else {
        glfwSetWindowMonitor(window, nullptr, 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
        isFullscreen = false;
    }
}

// Load cursor image (PNG, RGBA) using stb_image
GLFWcursor* Window::loadCursor(const char* path, int hotspotX, int hotspotY) {
    if (!window) return nullptr;

    int width, height, channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 4);
    if (!data) {
        std::cerr << "Failed to load cursor image: " << path << "\n";
        return nullptr;
    }

    GLFWimage image;
    image.width = width;
    image.height = height;
    image.pixels = data;

    if (cursor) {
        glfwDestroyCursor(cursor); // destroy previous cursor
        cursor = nullptr;
    }

    cursor = glfwCreateCursor(&image, hotspotX, hotspotY);
    stbi_image_free(data);

    if (!cursor) {
        std::cerr << "Failed to create GLFW cursor from image: " << path << "\n";
        return nullptr;
    }

    glfwSetCursor(window, cursor); // set cursor immediately
    return cursor;
}

// Static callbacks
void Window::s_window_size_cb(GLFWwindow* win, int w, int h) {
    WINDOW_WIDTH = w;
    WINDOW_HEIGHT = h;
    isResized = true;
}

void Window::s_window_focus_cb(GLFWwindow* win, int focused) {
    isFocused = (focused == GLFW_TRUE);
}
