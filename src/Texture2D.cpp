#include "Texture2D.hpp"

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

Texture2D::Texture2D(const uint8_t* buffer, size_t bufferSize, Texture2DType type, bool genMipmap, FilterMode filter)
    : ID(0), width(0), height(0), channels(0), type(type)
{
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load_from_memory(
        buffer,
        static_cast<int>(bufferSize),
        &width, &height, &channels, 0
    );

    if (!data) {
        Debug::logError("Failed to load texture from memory!");
        return;
    }

    GLenum format = pickFormat(channels);
    uploadToGPU(data, format, format, GL_UNSIGNED_BYTE, filter, genMipmap);
    stbi_image_free(data);
}

Texture2D::Texture2D(const uint8_t* data, int width, int height, int channels, Texture2DType type, bool genMipmap, FilterMode filter)
    : ID(0), width(width), height(height), channels(channels), type(type)
{
    GLenum format = pickFormat(channels);
    uploadToGPU(data, format, format, GL_UNSIGNED_BYTE, filter, genMipmap);
}

Texture2D::Texture2D(const uint8_t* data, int width, int height, Texture2DType type, bool genMipmap, GLenum internalFormat, GLenum dataFormat, GLenum dataType, FilterMode filter)
    : ID(0), width(width), height(height), channels(0), type(type)
{
    uploadToGPU(data, internalFormat, dataFormat, dataType, filter, genMipmap);
}

Texture2D::Texture2D(const char* path, bool alpha, Texture2DType type, bool genMipmap, FilterMode filter)
    : width(0), height(0), channels(0), ID(0), type(type)
{
    pathSource = std::string(path);
    loadFromFile(path, alpha, filter, genMipmap);
}

Texture2D::Texture2D(const std::string& path, bool alpha, Texture2DType type, bool genMipmap, FilterMode filter)
    : Texture2D(path.c_str(), alpha, type, genMipmap, filter) {
}

Texture2D::~Texture2D() {
    if (ID != 0) {
        glDeleteTextures(1, &ID);
    }
}

void Texture2D::bind(unsigned int slot) const {
    if (slot > 31) {
        Debug::logWarn("Texture slot " + std::to_string(slot) + " is out of range. Max is 31.");
    }
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, ID);
}

void Texture2D::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::loadFromFile(const char* path, bool alpha, FilterMode filter, bool createMipmap) {
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = nullptr;
    if (alpha) {
        data = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
        channels = 4;
    }
    else {
        data = stbi_load(path, &width, &height, &channels, 0);
    }

    if (!data) {
        Debug::logError("Failed to load texture: " + std::string(path));
        return;
    }

    GLenum format = pickFormat(channels);
    uploadToGPU(data, format, format, GL_UNSIGNED_BYTE, filter, createMipmap);
    stbi_image_free(data);
}

void Texture2D::uploadToGPU(const uint8_t* data, GLenum internalFormat, GLenum dataFormat, GLenum dataType, FilterMode filter, bool createMipmap) {
	useMipmap = createMipmap;
	this->internalFormat = internalFormat;
	this->dataFormat = dataFormat;
	this->dataType = dataType;
	this->filter = filter;

    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);

    // Wrapping & filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, createMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    // Upload
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, dataType, data);
    if (createMipmap) glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
}
