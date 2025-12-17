#pragma once

#include <glad/glad.h>
#include <memory>
#include <vector>

#include "Texture2D.hpp"
#include "ScreenQuad.hpp"

struct ColorAttachment
{
    std::unique_ptr<Texture2D> texture;
    GLenum attachmentIndex; // GL_COLOR_ATTACHMENT0 + i

    ColorAttachment(int width, int height, GLenum attachment, GLenum internalFormat, GLenum dataFormat, GLenum dataType, Texture2D::FilterMode filter, bool genMipmap)
        : attachmentIndex(attachment)
    {
        texture = std::make_unique<Texture2D>(
			nullptr, width, height, Texture2D::TextureRender, genMipmap, internalFormat, dataFormat, dataType, filter
        );
    }
};


class RenderTexture
{
public:
    enum ClearMode : uint32_t {
        NONE = 0,
        CLEAR_COLOR = 1 << 0,
        CLEAR_DEPTH = 1 << 1,
        CLEAR_STENCIL = 1 << 2,
        CLEAR_ALL = CLEAR_COLOR | CLEAR_STENCIL | CLEAR_DEPTH,
    };

    RenderTexture(int width, int height);
    ~RenderTexture();

    void addColorAttachment(GLenum internalFormat = GL_RGB, GLenum dataFormat = GL_RGB, GLenum dataType = GL_UNSIGNED_BYTE, Texture2D::FilterMode filter = Texture2D::FilterNearest, bool generateMipmap = false);
	void useDepthTexture(Texture2D::FilterMode filter = Texture2D::FilterNearest, GLenum depthInternalFormat = GL_DEPTH_COMPONENT24, GLenum depthDataFormat = GL_DEPTH_COMPONENT, GLenum depthDataType = GL_UNSIGNED_BYTE);
    void useDepthRBO();

    void bind(ClearMode clearMode = CLEAR_ALL);
	void resize(int newWidth, int newHeight);
    static void unbind();

    const std::vector<ColorAttachment>& getColorAttachments() const { return attachments; }
    Texture2D* getDepthTexture() const;

private:
    GLuint FBO = 0;
    GLuint RBO = 0;

    int width, height;
    std::vector<ColorAttachment> attachments;
    std::unique_ptr<Texture2D> depth;

    void updateDrawBuffers();
    void destroy();
};
