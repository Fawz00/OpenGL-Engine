#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <unordered_set>
#include <cstring>
#include <string>

static class GLExtensionLoader {
	public:
		static bool Initialize();

		static bool HasExtension(const char* extensionName);
		static const std::unordered_set<std::string>& GetExtensions();

	private:
		~GLExtensionLoader() = default;

		static std::unordered_set<std::string> m_extensions;
};