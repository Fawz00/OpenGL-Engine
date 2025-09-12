#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Input.hpp"
#include "Shader.hpp"
#include "Texture2D.hpp"
#include "Camera.hpp"
#include "Window.hpp"
#include "AppTime.hpp"

class EngineRenderer {
private:
	~EngineRenderer();
	static inline unsigned int VBO, VAO, EBO;
	static inline Texture2D* texture = nullptr;
	static inline Shader* shader = nullptr;
	static inline Camera* camera = nullptr;
	static void onInit();
	static void onWindowResize();
	static void onUpdate();
	static void onDestroy();
	friend int main();
};