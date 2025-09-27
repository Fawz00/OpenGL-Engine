#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "ScreenQuad.hpp"
#include "RenderTexture.hpp"
#include "Input.hpp"
#include "Shader.hpp"
#include "Texture2D.hpp"
#include "Camera.hpp"
#include "Window.hpp"
#include "Time.hpp"
#include "Model.hpp"
#include "TextureCubeMap.hpp"
#include "Animation.hpp"
#include "Animator.hpp"

class EngineRenderer {
	private:
		~EngineRenderer() = default;

		static inline Animation* danceAnimation= nullptr;
		static inline Animator* animator= nullptr;

		static inline TextureCubeMap* skybox = nullptr;

		static inline RenderTexture* renderTexture = nullptr;
		
		static inline std::vector<Model*> models;
		static inline std::vector<glm::mat4> modelMatrices;

		static inline Shader* skyboxShader = nullptr;
		static inline Shader* shader = nullptr;
		static inline Shader* quadShader = nullptr;
		static inline Camera* camera = nullptr;

		static void onInit();
		static void onWindowResize();
		static void onUpdate();
		static void onDestroy();
		friend int main();
};