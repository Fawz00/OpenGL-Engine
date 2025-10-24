#pragma once

#include <string>
#include <vector>
#include <gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "ShaderStage.hpp"
#include "Debug.hpp"

class Shader {
private:
    GLuint program = 0;

    bool linked;
    std::vector<ShaderStage> stages;
    std::vector<std::string> globalDefines;

public:
    Shader() : program(0), linked(false) { } // Also initialize in constructor
	~Shader();

    Shader& attachShader(ShaderType type, const std::string& path, std::initializer_list<std::string> defines = {});
    Shader& define(const std::string& macro);

    Shader& link();

    void bind();
    static void unbind();

    GLint genAttrId(const std::string& name);
    static void setAttr(GLint id, GLint size, GLsizei stride, const void* offset, GLenum type = GL_FLOAT);
    static void disableAttr(GLint id);
	void setUInt(const std::string& name, unsigned int value);
    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void setVec2(const std::string& name, const float* data);
    void setVec2(const std::string& name, const glm::vec2 &data);
    void setVec3(const std::string& name, const float* data);
	void setVec3(const std::string& name, const glm::vec3 &data);
    void setVec4(const std::string& name, const float* data);
	void setVec4(const std::string& name, const glm::vec4 &data);
    void setMat4(const std::string& name, const float* data);
	void setMat4(const std::string& name, const glm::mat4 &mat);

    GLuint getID() const { return program; }
};
