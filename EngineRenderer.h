#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Shader.h"
#include "Texture.h"

class EngineRenderer {
	private:
		~EngineRenderer();
		static Texture* texture;
		static Shader* shader;
		static void onInit();
		static void onUpdate(float deltaTime);
		friend int main();
};