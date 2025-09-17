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

Texture2D::Texture2D(const uint8_t* buffer, size_t bufferSize, TextureType type)
    : ID(0), width(0), height(0), channels(0), type(type)
{
    stbi_set_flip_vertically_on_load(false);
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
    uploadToGPU(data, format);
    stbi_image_free(data);
}

Texture2D::Texture2D(const uint8_t* data, int width, int height, int channels, TextureType type)
    : ID(0), width(width), height(height), channels(channels), type(type)
{
    Debug::log("Creating texture from raw data: " +
        std::to_string(width) + "x" + std::to_string(height) +
        " with " + std::to_string(channels) + " channels");

    GLenum format = pickFormat(channels);
    uploadToGPU(data, format);
}

Texture2D::Texture2D(const char* path, bool alpha, TextureType type)
    : width(0), height(0), channels(0), ID(0), type(type)
{
    pathSource = std::string(path);
    loadFromFile(path, alpha);
}

Texture2D::Texture2D(const std::string& path, bool alpha, TextureType type)
    : Texture2D(path.c_str(), alpha, type) {
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

void Texture2D::loadFromFile(const char* path, bool alpha) {
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
    uploadToGPU(data, format);
    stbi_image_free(data);
}

void Texture2D::uploadToGPU(const uint8_t* data, GLenum format) {
    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);

    // Wrapping & filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Upload
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
}
