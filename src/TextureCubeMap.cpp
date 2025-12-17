#include "TextureCubeMap.hpp"

namespace {
	GLenum pickFormat(int channels) {
		switch (channels) {
			case 1: return GL_RED;
			case 3: return GL_RGB;
			case 4: return GL_RGBA;
			default: return GL_RGB; // fallback
		}
	}
}

TextureCubeMap::TextureCubeMap(const char* faces[6], TextureCubeMapType type)
	: ID(0), width(0), height(0), type(type)
{
	loadFromFiles(faces);
}

TextureCubeMap::TextureCubeMap(const std::string faces[6], TextureCubeMapType type)
	: ID(0), width(0), height(0), type(type)
{
	const char* cfaces[6];
	for (int i = 0; i < 6; ++i) {
		cfaces[i] = faces[i].c_str();
	}
	loadFromFiles(cfaces);
}

TextureCubeMap::~TextureCubeMap() {
	if (ID != 0) {
		glDeleteTextures(1, &ID);
	}
}

void TextureCubeMap::bind(unsigned int slot) const {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_CUBE_MAP, ID);
}

void TextureCubeMap::unbind() {
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void TextureCubeMap::setPath(const std::string paths[6]) {
	for (int i = 0; i < 6; i++) {
		pathSources[i] = paths[i];
	}
}

void TextureCubeMap::loadFromFiles(const char* faces[6]) {
	stbi_set_flip_vertically_on_load(false);
	glGenTextures(1, &ID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, ID);
	for (int i = 0; i < 6; i++) {
		pathSources[i] = std::string(faces[i]);
		int w, h, channels;
		unsigned char* data = stbi_load(faces[i], &w, &h, &channels, 0);
		if (data) {
			if (i == 0) { // Set width and height from first face
				width = w;
				height = h;
			}
			else {
				if (w != width || h != height) {
					Debug::logWarn("Cubemap face " + std::string(faces[i]) +
						" has different dimensions (" + std::to_string(w) + "x" + std::to_string(h) +
						") than the first face (" + std::to_string(width) + "x" + std::to_string(height) + ").");
				}
			}
			GLenum format = pickFormat(channels);
			uploadToGPU(data, w, h, format, i);
			stbi_image_free(data);
		}
		else {
			Debug::logError("Failed to load cubemap texture at path: " + std::string(faces[i]));
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void TextureCubeMap::uploadToGPU(const uint8_t* data, int width, int height, GLenum format, int faceIndex) {
	glTexImage2D(
		GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex,
		0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data
	);
}
