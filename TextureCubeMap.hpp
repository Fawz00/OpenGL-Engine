#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "stb_image.h"
#include "I_Texture.hpp"

#include "Debug.hpp"

class TextureCubeMap : public I_Texture {
public:
	enum TextureCubeMapType {
		CubeMapOther,
		Skybox
	};

	// Constructors
	TextureCubeMap(const std::string faces[6], TextureCubeMapType type = CubeMapOther);
	TextureCubeMap(const char* faces[6], TextureCubeMapType type = CubeMapOther);

	~TextureCubeMap();

	void bind(unsigned int slot = 0) const override;
	static void unbind();

	inline int getWidth() const override { return width; }
	inline int getHeight() const { return height; }
	inline int getSize() const override { return width * height * 6 * channels; } // Assuming 4 channels (RGBA)
	inline unsigned int getId() const override { return ID; }
	inline TextureCubeMapType getType() const { return type; }
	void setPath(const std::string paths[6]);
	inline std::string* getPaths() { return pathSources; }
	inline std::string getPath() const override { return pathSources[0]; } // Implement abstract method

private:
	unsigned int ID;
	int width, height, channels;
	TextureCubeMapType type;
	std::string pathSources[6];

	// Helpers
	void loadFromFiles(const char* faces[6]);
	void uploadToGPU(const uint8_t* data, int width, int height, GLenum format, int faceIndex);
};