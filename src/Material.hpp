#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Shader.hpp"
#include "Texture2D.hpp"

enum Texture2DFlags : uint32_t {
    HasDiffuse = 1 << 0,
    HasSpecular = 1 << 1,
    HasNormal = 1 << 2,
    HasHeight = 1 << 3,
};

/**
 * Material: minimal set of texture slots (textures only for now).
 *
 * The material does not own textures; they are owned by the Model that built it.
 * Sampler names follow the previous convention: texture_diffuse1, texture_diffuse2,
 * texture_specular1, texture_normal1, texture_height1, texture_other1 ...
 * Texture units are assigned in the order textures were added.
 *
 * Uniform names are built once in addTexture(), so bind() does no string building.
 */
class Material {
public:
    enum class Slot : uint8_t { Diffuse, Specular, Normal, Height, Other };

    /** Adds a texture to a slot. A null texture is ignored. Call only while building. */
    void addTexture(Slot slot, const Texture2D* texture);

    /** Binds all textures to consecutive units, sets sampler uniforms and `uTextureFlags`. */
    void bind(Shader& shader) const;

    /** Sets `uTextureFlags` to 0 (for meshes without material). */
    static void bindNone(Shader& shader);

    /** Restores the active texture unit to 0 and unbinds the 2D texture there. */
    static void unbindAll();

    [[nodiscard]] uint32_t getFlags() const noexcept { return m_Flags; }
    [[nodiscard]] size_t getTextureCount() const noexcept { return m_Bindings.size(); }

private:
    struct Binding {
        const Texture2D* texture;
        std::string uniformName;
    };

    std::vector<Binding> m_Bindings;
    uint32_t m_Flags = 0;
    uint32_t m_SlotCount[5] = {};
};