#include "GLExtensionLoader.hpp"

std::unordered_set<std::string> GLExtensionLoader::m_extensions;

bool GLExtensionLoader::Initialize()
{
    GLint count = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &count);

    m_extensions.reserve(count);

    for (GLint i = 0; i < count; ++i)
    {
        const char* ext =
            reinterpret_cast<const char*>(
                glGetStringi(GL_EXTENSIONS, i)
                );

        if (ext)
            m_extensions.emplace(ext);
    }

    return true;
}

bool GLExtensionLoader::HasExtension(const char* extensionName)
{
    if (!extensionName)
        return false;

    return m_extensions.contains(extensionName);
}

const std::unordered_set<std::string>& GLExtensionLoader::GetExtensions()
{
     return m_extensions;
}