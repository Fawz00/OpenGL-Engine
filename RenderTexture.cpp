#include "RenderTexture.hpp"

RenderTexture::RenderTexture(int width, int height)
    : width(width), height(height)
{
    glGenFramebuffers(1, &FBO);
}

RenderTexture::~RenderTexture()
{
    destroy();
}

void RenderTexture::addColorAttachment(GLenum internalFormat, GLenum dataFormat, GLenum dataType, Texture2D::FilterMode filter, bool generateMipmap)
{
    if (depth || RBO) {
        Debug::logWarn("addColorAttachment: Adding color attachments after depth attachment or RBO may lead to incomplete framebuffer.");
	}

    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    GLenum attachment = GL_COLOR_ATTACHMENT0 + (GLenum)attachments.size();

    attachments.emplace_back(width, height, attachment, internalFormat, dataFormat, dataType, filter, generateMipmap);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        attachment,
        GL_TEXTURE_2D,
        attachments.back().texture->getId(),
        0
    );

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        Debug::logError("addColorAttachment: Framebuffer not complete!");
    }

    updateDrawBuffers();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderTexture::useDepthTexture(GLenum depthInternalFormat, GLenum depthDataFormat, GLenum depthDataType)
{
	if (RBO) {
		Debug::logWarn("useDepthTexture: Depth RBO already exists, destroying it.");
        glDeleteRenderbuffers(1, &RBO);
		RBO = 0;
	}

    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    depth = std::make_unique<Texture2D>(nullptr, width, height, Texture2D::TextureRender, false, depthInternalFormat, depthDataFormat, depthDataType, Texture2D::FilterNearest);
	depth->bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth->getId(), 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        Debug::logError("useDepthTexture: Framebuffer not complete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	depth->unbind();
}

void RenderTexture::useDepthRBO()
{
    if (depth) {
		Debug::logWarn("useDepthRBO: Depth texture already exists, cannot use RBO.");
		return;
	}

    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    glGenRenderbuffers(1, &RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        Debug::logError("useDepthRBO: Framebuffer not complete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void RenderTexture::resize(int newWidth, int newHeight)
{
    if (newWidth == width && newHeight == height)
        return;
    width = newWidth;
    height = newHeight;

    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    // Resize color attachments
    for (auto& att : attachments) {
        att.texture->bind();
        glTexImage2D(GL_TEXTURE_2D, 0, att.texture->getInternalFormat(), width, height, 0, att.texture->getDataFormat(), att.texture->getDataType(), nullptr);
        if (att.texture->hasMipmap()) glGenerateMipmap(GL_TEXTURE_2D);

        glFramebufferTexture2D(GL_FRAMEBUFFER, att.attachmentIndex, GL_TEXTURE_2D, att.texture->getId(), 0);
    }

    // Resize depth texture
    if (depth) {
        depth->bind();
        glTexImage2D(GL_TEXTURE_2D, 0, depth->getInternalFormat(), width, height, 0, depth->getDataFormat(), depth->getDataType(), nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth->getId(), 0);
    }

    // Resize RBO if exists
    if (RBO) {
        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);
    }

    updateDrawBuffers();

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        Debug::logError("resize: Framebuffer not complete!");
    }

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	Texture2D::unbind();
}

Texture2D* RenderTexture::getDepthTexture() const
{
    if (depth)
		return depth.get();
    else
    {
		Debug::logWarn("RenderTexture has no depth texture.");
		return nullptr;
    }
}

void RenderTexture::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glViewport(0, 0, width, height);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderTexture::unbind()
{
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderTexture::updateDrawBuffers()
{
    std::vector<GLenum> drawBuffers;
    for (size_t i = 0; i < attachments.size(); ++i)
        drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + (GLenum)i);

    glDrawBuffers((GLsizei)drawBuffers.size(), drawBuffers.data());
}

void RenderTexture::destroy()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

    if (RBO) { glDeleteRenderbuffers(1, &RBO); RBO = 0; }
    if (FBO) { glDeleteFramebuffers(1, &FBO); FBO = 0; }
    attachments.clear();
    depth.reset();
}