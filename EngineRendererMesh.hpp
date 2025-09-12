#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Input.hpp"
#include "Shader.hpp"
#include "Texture2D.hpp"
#include "Camera.hpp"
#include "Window.hpp"
#include "AppTime.hpp"
#include "Model.hpp"

class EngineRendererMesh {
	private:
		~EngineRendererMesh();
		static inline Texture2D* texture = nullptr;
		static inline Model* ourModel = nullptr;
		static inline Mesh* mesh = nullptr;
		static inline Shader* shader = nullptr;
		static inline Camera* camera = nullptr;
		static void onInit();
		static void onWindowResize();
		static void onUpdate();
		static void onDestroy();
		friend int main();
};