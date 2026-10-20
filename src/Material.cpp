#include "Material.hpp"

namespace {
    // Built once: avoids constructing a std::string on every bind.
    const std::string kTextureFlagsUniform = "uTextureFlags";
}

void Material::addTexture(Slot slot, const Texture2D* texture)
{
    if (!texture) return;

    const size_t index = static_cast<size_t>(slot);
    const char* prefix = "texture_other";

    switch (slot) {
    case Slot::Diffuse:  prefix = "texture_diffuse";  m_Flags |= HasDiffuse;  break;
    case Slot::Specular: prefix = "texture_specular"; m_Flags |= HasSpecular; break;
    case Slot::Normal:   prefix = "texture_normal";   m_Flags |= HasNormal;   break;
    case Slot::Height:   prefix = "texture_height";   m_Flags |= HasHeight;   break;
    case Slot::Other:    break;
    }

    m_Bindings.push_back({ texture, std::string(prefix) + std::to_string(++m_SlotCount[index]) });
}

void Material::bind(Shader& shader) const
{
    for (size_t i = 0; i < m_Bindings.size(); ++i) {
        const Binding& b = m_Bindings[i];
        shader.setInt(b.uniformName, static_cast<int>(i));
        b.texture->bind(static_cast<unsigned int>(i));
    }
    shader.setUInt(kTextureFlagsUniform, m_Flags);
}

void Material::bindNone(Shader& shader)
{
    shader.setUInt(kTextureFlagsUniform, 0);
}

void Material::unbindAll()
{
    glActiveTexture(GL_TEXTURE0);
    Texture2D::unbind();
}