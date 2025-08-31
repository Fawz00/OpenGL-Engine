#include "Texture.h"

Texture::Texture(const char* path, bool alpha) : width(0), height(0), channels(0), ID(0) {
	stbi_set_flip_vertically_on_load(true);
	if (alpha) {
		data = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
	}
	else {
		data = stbi_load(path, &width, &height, &channels, 0);
	}
	if (!data) {
		Debug::logError("Failed to load texture: " + std::string(path));
		return;
	}

	glGenTextures(1, &ID);
	glBindTexture(GL_TEXTURE_2D, ID);
	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Upload texture data
	if (alpha) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else {
		if (channels == 3) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else if (channels == 4) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		else {
			std::cerr << "Unsupported number of channels: " << channels << " in texture: " << path << "\n";
			Debug::logError("Unsupported number of channels: " + std::to_string(channels) + " in texture: " + std::string(path));
			stbi_image_free(data);
			return;
		}
	}
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(data);
}

Texture::~Texture() {
	glDeleteTextures(1, &ID);
}

void Texture::bind(unsigned int slot) const {
	if (slot > 31) {
		Debug::logWarn("Texture slot " + std::to_string(slot) + " is out of range. Max is 31.");
	}
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, ID);
}

void Texture::unbind() {
	glBindTexture(GL_TEXTURE_2D, 0);
}
