#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include "Debug.h"

class Texture {
public:
	Texture(const char* path, bool alpha = false);
	~Texture();
	void bind(unsigned int slot = 0) const;
	static void unbind();
	inline int getWidth() const { return width; }
	inline int getHeight() const { return height; }
	inline unsigned int getID() const { return ID; }

private:
	uint8_t *data;
	unsigned int ID;
	int width, height, channels;
};