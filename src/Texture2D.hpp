#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "stb_image.h"
#include "I_Texture.hpp"

#include "Debug.hpp"

class Texture2D : public I_Texture {
public:
	enum Texture2DType {
		TextureOther,
		TextureRender,
		TextureDiffuse,
		TextureSpecular,
		TextureNormal,
		TextureHeight
	};

	// Constructors
	Texture2D(const std::string& path, bool alpha = false, Texture2DType type = TextureOther, bool genMipmap = false, FilterMode filter = FilterLinear);
	Texture2D(const char* path, bool alpha = false, Texture2DType type = TextureOther, bool genMipmap = false, FilterMode filter = FilterLinear);
	Texture2D(const uint8_t* buffer, size_t bufferSize, Texture2DType type = TextureOther, bool genMipmap = false, FilterMode filter = FilterLinear);
	Texture2D(const uint8_t* data, int width, int height, int channels, Texture2DType type = TextureOther, bool genMipmap = false, FilterMode filter = FilterLinear);
	Texture2D(const uint8_t* data, int width, int height, Texture2DType type = TextureOther, bool genMipmap = false, GLenum internalFormat = GL_RGB, GLenum dataFormat = GL_RGB, GLenum dataType = GL_UNSIGNED_BYTE, FilterMode filter = FilterLinear);

	~Texture2D();
	
	void bind(unsigned int slot = 0) const;
	static void unbind();
	
	inline int* getDimension() const { return new int[2]{width, height}; }
	inline int getSize() const { return width * height * channels; }
	inline unsigned int getId() const
	{
		if (ID == 0)
			Debug::logWarn("Texture not created properly, ID is 0.");
		return ID;
	}
	inline Texture2DType getType() const { return type; }
	void setPath(const std::string& path) { pathSource = path; }
	inline std::string getPath() const { return pathSource; }
	inline GLenum getInternalFormat() const { return internalFormat; }
	inline GLenum getDataFormat() const { return dataFormat; }
	inline FilterMode getFilter() const { return filter; }
	inline GLenum getDataType() const { return dataType; }
	inline bool hasMipmap() const { return useMipmap; }

private:
	unsigned int ID;
	int width, height, channels;
	GLenum internalFormat = GL_RGB;
	GLenum dataFormat = GL_RGB;
	GLenum dataType = GL_UNSIGNED_BYTE;
	FilterMode filter = FilterLinear;
	bool useMipmap = false;
	Texture2DType type;
	std::string pathSource;

	// Helpers
	void loadFromFile(const char* path, bool alpha, FilterMode filter = FilterLinear, bool createMipmap = false);
	void uploadToGPU(const uint8_t* data, GLenum internalFormat, GLenum dataFormat, GLenum dataType = GL_UNSIGNED_BYTE, FilterMode filter = FilterLinear, bool createMipmap = false);
};