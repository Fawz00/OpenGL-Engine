#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Input.hpp"
#include "Shader.hpp"
#include "Texture2D.hpp"
#include "Camera.hpp"
#include "Window.hpp"
#include "Time.hpp"
#include "Model.hpp"

class EngineRenderer {
	private:
		~EngineRenderer();
		static inline std::vector<Model*> models;
		static inline Shader* shader = nullptr;
		static inline Camera* camera = nullptr;
		static void onInit();
		static void onWindowResize();
		static void onUpdate();
		static void onDestroy();
		friend int main();
};