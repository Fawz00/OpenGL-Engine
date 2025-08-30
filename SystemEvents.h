#pragma once

struct WindowResizeEvent {
	int width;
	int height;
	WindowResizeEvent(GLFWwindow* win, int w, int h) : width(w), height(h) {}
};
struct WindowFocusEvent {
	bool focused;
	WindowFocusEvent(bool f) : focused(f) {}
};
struct WindowCloseEvent {
	WindowCloseEvent() {}
};

struct KeyEvent {
	int key;
	int scancode;
	int action;
	int mods;
	KeyEvent(int k, int s, int a, int m) : key(k), scancode(s), action(a), mods(m) {}
};
struct CharEvent {
	unsigned int codepoint;
	CharEvent(unsigned int cp) : codepoint(cp) {}
};
struct CursorPosEvent {
	double xpos;
	double ypos;
	CursorPosEvent(double x, double y) : xpos(x), ypos(y) {}
};
struct MouseButtonEvent {
	int button;
	int action;
	int mods;
	MouseButtonEvent(int b, int a, int m) : button(b), action(a), mods(m) {}
};
struct MouseScrollEvent {
	double offsetx;
	double offsety;
	MouseScrollEvent(double ox, double oy) : offsetx(ox), offsety(oy) {}
};