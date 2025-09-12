#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"

#include "Debug.hpp"

class Texture2D {
public:
	static enum class TextureType {
		Other,
		TextureDiffuse,
		TextureSpecular,
		TextureNormal,
		TextureHeight
	};

	Texture2D(const std::string& path, bool alpha = false, TextureType type = TextureType::Other);
	Texture2D(const char* path, bool alpha = false, TextureType type = TextureType::Other);
	~Texture2D();
	void bind(unsigned int slot = 0) const;
	static void unbind();
	inline int getWidth() const { return width; }
	inline int getHeight() const { return height; }
	inline unsigned int getId() const { return ID; }
	inline TextureType getType() const { return type; }
	inline std::string getPath() const { return pathSource; }

private:
	uint8_t *data;
	unsigned int ID;
	int width, height, channels;
	TextureType type;
	std::string pathSource;
};