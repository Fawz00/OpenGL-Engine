#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Shader.h"
#include "Texture.h"
#include "Camera.h"
#include "Window.h"
#include "AppTime.h"

class EngineRenderer {
	private:
		~EngineRenderer();
		static inline unsigned int VBO, VAO, EBO;
		static inline Texture* texture = nullptr;
		static inline Shader* shader = nullptr;
		static inline Camera* camera = nullptr;
		static void onInit();
		static void onUpdate();
		static void onDestroy();
		friend int main();
};