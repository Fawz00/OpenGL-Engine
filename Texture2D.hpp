#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"

#include "Debug.hpp"

class Texture2D {
public:
	enum TextureType {
		TextureOther,
		TextureDiffuse,
		TextureSpecular,
		TextureNormal,
		TextureHeight
	};

	enum TextureFlags : uint32_t {
		HasDiffuse  = 1 << 0,
		HasSpecular = 1 << 1,
		HasNormal   = 1 << 2,
		HasHeight   = 1 << 3,
	};

	// Constructors
	Texture2D(const std::string& path, bool alpha = false, TextureType type = TextureOther);
	Texture2D(const char* path, bool alpha = false, TextureType type = TextureOther);
	Texture2D(const uint8_t* buffer, size_t bufferSize, TextureType type);
	Texture2D(const uint8_t* data, int width, int height, int channels = 4, TextureType type = TextureOther);

	~Texture2D();
	
	void bind(unsigned int slot = 0) const;
	static void unbind();
	
	inline int getWidth() const { return width; }
	inline int getHeight() const { return height; }
	inline unsigned int getId() const { return ID; }
	inline TextureType getType() const { return type; }
	void setPath(const std::string& path) { pathSource = path; }
	inline std::string getPath() const { return pathSource; }

private:
	unsigned int ID;
	int width, height, channels;
	TextureType type;
	std::string pathSource;

	// Helpers
	void loadFromFile(const char* path, bool alpha);
	void uploadToGPU(const uint8_t* data, GLenum format);
};