// window_port.cpp
// Compile example (Linux):
// g++ window_port.cpp -o window_port -lglfw -ldl -lpthread -lX11 -lXrandr -lXi -lXxf86vm -lXinerama -lXcursor
// Make sure glad.c is compiled/linked or use glad as library. For simplicity use glad as header-only loader if available.

#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "stb_image.h"
#include "Debug.cpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Window {
    public:
        Window(int width, int height, const std::string& title)
            : WINDOW_WIDTH(width), WINDOW_HEIGHT(height), title(title) {
        }

        void create(bool maximized = false, bool vsync = true) {
            if (!glfwInit()) {
				Debug::logError("Unable to initialize GLFW");
                return;
            }

            // Hints
            if (maximized) glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

            #ifdef __APPLE__
                glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
            #endif
            // doublebuffer is default true

            window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, title.c_str(), nullptr, nullptr);
            if (!window) {
				Debug::logError("Failed to create GLFW window");
                glfwTerminate();
                return;
            }

            // Center window if not maximized
            if (!maximized) {
                const GLFWvidmode* vm = glfwGetVideoMode(glfwGetPrimaryMonitor());
                if (vm) {
                    windowPos[0] = (vm->width - WINDOW_WIDTH) / 2;
                    windowPos[1] = (vm->height - WINDOW_HEIGHT) / 2;
                    glfwSetWindowPos(window, windowPos[0], windowPos[1]);
                }
            }

            // store user pointer for callbacks
            glfwSetWindowUserPointer(window, this);

            // Setup callbacks (static wrappers call instance methods)
            glfwSetWindowSizeCallback(window, &Window::s_window_size_cb);
            glfwSetWindowFocusCallback(window, &Window::s_window_focus_cb);

            // Make context current and load GL
            glfwMakeContextCurrent(window);
            if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
				Debug::logError("Failed to initialize GLAD");
                return;
            }

            std::string version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
			Debug::log("OpenGL Renderer: " + version);

            // Vsync
            glfwSwapInterval(vsync ? 1 : 0);
            this->vsync = vsync;

            // Set initial cursor position to center
            glfwSetCursorPos(window, WINDOW_WIDTH / 2.0, WINDOW_HEIGHT / 2.0);

            // Query actual window size (may be affected by window manager)
            int w = 0, h = 0;
            glfwGetWindowSize(window, &w, &h);
            WINDOW_WIDTH = w; WINDOW_HEIGHT = h;

            this->isFocused = true;
            this->isResized = false;

			initialized = true;
            return;
        }

        void update() {
            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        void destroy() {
            if (cursor) {
                glfwDestroyCursor(cursor);
                cursor = nullptr;
            }
            if (window) {
                glfwSetWindowUserPointer(window, nullptr);
                glfwDestroyWindow(window);
                window = nullptr;
            }
            glfwTerminate();
        }

        GLFWwindow* getGLFWwindow() const {
            return window;
        }
        int getWidth() const { return WINDOW_LAST_WIDTH; }
        int getHeight() const { return WINDOW_HEIGHT; }

        bool shouldClose() const {
            return window ? glfwWindowShouldClose(window) : true;
        }

        void setShouldClose(bool v) {
            if (window) glfwSetWindowShouldClose(window, v ? GLFW_TRUE : GLFW_FALSE);
        }

        bool focused() const { return isFocused; }
        bool resized() const { return isResized; }
        void setResized(bool b) { isResized = b; }
        int width() const { return WINDOW_WIDTH; }
        int height() const { return WINDOW_HEIGHT; }
		bool isVsync() const { return vsync; }
		bool isInitialized() const { return initialized; }

        // Toggle fullscreen similar to Java code: store last window pos/size and restore later
        void toggleFullscreen() {
            if (!window) return;
            const GLFWvidmode* vm = glfwGetVideoMode(glfwGetPrimaryMonitor());
            if (!vm) return;

            if (!isFullscreen) {
                // backup last size & pos
                int w, h, px, py;
                glfwGetWindowSize(window, &w, &h);
                glfwGetWindowPos(window, &px, &py);
                WINDOW_LAST_WIDTH = w;
                WINDOW_LAST_HEIGHT = h;
                WINDOW_LAST_POSX = px;
                WINDOW_LAST_POSY = py;

                glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, vm->width, vm->height, vm->refreshRate);
                isFullscreen = true;
            }
            else {
                glfwSetWindowMonitor(window, nullptr, WINDOW_LAST_POSX, WINDOW_LAST_POSY, WINDOW_LAST_WIDTH, WINDOW_LAST_HEIGHT, 0);
                isFullscreen = false;
            }
        }

        // Load cursor image (PNG, RGBA) using stb_image
        GLFWcursor* loadCursor(const char* path, int hotspotX = 0, int hotspotY = 0)
        {
            int width, height, channels;
            unsigned char* data = stbi_load(path, &width, &height, &channels, 4);
            if (!data) {
				Debug::logError("Failed to load cursor image: " + std::string(path));
                return nullptr;
            }
            Debug::log("Loaded cursor image: " + std::string(path) +
				" (" + std::to_string(width) + "x" + std::to_string(height) + ", channels=" + std::to_string(channels) + ")");

            GLFWimage image;
            image.width = width;
            image.height = height;
            image.pixels = data;

            GLFWcursor* cursor = glfwCreateCursor(&image, hotspotX, hotspotY);
            if (!cursor) {
				Debug::logError("Failed to create GLFW cursor from image: " + std::string(path));
                stbi_image_free(data);
                return nullptr;
            }

            Debug::log("Successfully created cursor (" +
                std::to_string(width) + "x" + std::to_string(height) + "), hotspot=(" +
				std::to_string(hotspotX) + "," + std::to_string(hotspotY) + ")");

            stbi_image_free(data);
            return cursor;
        }

    private:
        GLFWwindow* window = nullptr;
        GLFWcursor* cursor = nullptr;

		bool initialized = false;

        int WINDOW_WIDTH;
        int WINDOW_HEIGHT;
        int WINDOW_LAST_WIDTH = 0;
        int WINDOW_LAST_HEIGHT = 0;
        int WINDOW_LAST_POSX = 0;
        int WINDOW_LAST_POSY = 0;

        std::string title;
        int windowPos[2]{ 0,0 };
        bool isFullscreen = false;
        bool isResized = false;
        bool isFocused = true;
        bool vsync = true;

        // Static callbacks to forward to instance via user pointer
        static void s_window_size_cb(GLFWwindow* win, int w, int h) {
            Window* self = static_cast<Window*>(glfwGetWindowUserPointer(win));
            if (!self) return;
            self->WINDOW_WIDTH = w;
            self->WINDOW_HEIGHT = h;
            self->isResized = true;
            // If you need to trigger viewport change here, do glViewport(0,0,w,h) from main thread when handling resize
        }

        static void s_window_focus_cb(GLFWwindow* win, int focused) {
            Window* self = static_cast<Window*>(glfwGetWindowUserPointer(win));
            if (!self) return;
            self->isFocused = (focused == GLFW_TRUE);
        }

        static void APIENTRY gl_debug_cb(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
            const GLchar* message, const void* userParam) {
            // Filter non-significant messages if you want
            std::cerr << "[GL DEBUG] severity=" << severity << " type=" << type << " id=" << id << " msg=" << message << std::endl;
        }
};
