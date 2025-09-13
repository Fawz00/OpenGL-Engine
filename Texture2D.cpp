#include "Texture2D.hpp"

Texture2D::Texture2D(uint8_t* buffer, size_t bufferSize, TextureType type)
    : ID(0), width(0), height(0), channels(0), type(type)
{
    stbi_set_flip_vertically_on_load(false);
    data = stbi_load_from_memory(
        buffer,
        static_cast<int>(bufferSize),
        &width,
        &height,
        &channels,
        0
    );

    if (!data) {
        Debug::log("Failed to load texture from memory!");
        return;
    }

    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);

    GLenum format = GL_RGB;
    if (channels == 1)      format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Wrapping & filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
}

Texture2D::Texture2D(uint8_t* data, int width, int height, int channels, TextureType type)
    : data(data), width(width), height(height), channels(channels), ID(0), type(type)
{
    Debug::log(
        "Creating texture from memory: " +
        std::to_string(width) + "x" + std::to_string(height) +
        " with " + std::to_string(channels) +
        " channels, Type: " + std::to_string(static_cast<int>(type))
    );

    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);

    GLenum format = GL_RGB;
    if (channels == 1)      format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    // Wrapping & filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Catatan: jangan free data, karena milik user
}

Texture2D::Texture2D(const char* path, bool alpha, TextureType type)
    : width(0), height(0), channels(0), ID(0), type(type)
{
    loadFromFile(path, alpha);

    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);

    // Wrapping & filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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
            Debug::logError(
                "Unsupported number of channels: " +
                std::to_string(channels) +
                " in texture: " + std::string(path)
            );
            stbi_image_free(data);
            return;
        }
    }

    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
}

Texture2D::Texture2D(const std::string& path, bool alpha, TextureType type)
    : Texture2D(path.c_str(), alpha, type)
{
    // delegating constructor
}

Texture2D::~Texture2D() {
    glDeleteTextures(1, &ID);
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
    stbi_set_flip_vertically_on_load(false);

    if (alpha) {
        data = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
    }
    else {
        data = stbi_load(path, &width, &height, &channels, 0);
    }

    if (!data) {
        Debug::logError("Failed to load texture: " + std::string(path));
    }
}
