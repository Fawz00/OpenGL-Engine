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
    std::unordered_map<std::string, GLint> attributeLocations;
    std::unordered_map<std::string, GLint> uniformLocations;

    GLint getCachedUniformLocation(const std::string& name);
	GLint getCachedAttributeLocation(const std::string& name);

public:
    Shader() : program(0), linked(false) { } // Also initialize in constructor
    ~Shader();

    // Move constructor
    Shader(Shader&& other) noexcept
        : program(other.program),
          linked(other.linked),
          stages(std::move(other.stages)),
          globalDefines(std::move(other.globalDefines)),
          attributeLocations(std::move(other.attributeLocations)),
          uniformLocations(std::move(other.uniformLocations))
    {
        other.program = 0;
        other.linked = false;
    }

    // Move assignment
    Shader& operator=(Shader&& other) noexcept
    {
        if (this != &other) {
            // Release current program if any
            if (program != 0) {
                glDeleteProgram(program);
            }

            program = other.program;
            linked = other.linked;
            stages = std::move(other.stages);
            globalDefines = std::move(other.globalDefines);
            attributeLocations = std::move(other.attributeLocations);
            uniformLocations = std::move(other.uniformLocations);

            other.program = 0;
            other.linked = false;
        }
        return *this;
    }

    Shader& attachShader(ShaderType type, const std::string& path, std::initializer_list<std::string> defines = {});
    Shader& define(const std::string& macro);

    Shader& link();

    void bind();
    static void unbind();

    static void setAttr(GLint id, GLint size, GLsizei stride, const void* offset, GLenum type = GL_FLOAT, GLboolean normalized = GL_FALSE);
    void setAttr(const std::string& name, GLint size, GLsizei stride, const void* offset, GLenum type = GL_FLOAT, GLboolean normalized = GL_FALSE);
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
    void setMat3(const std::string& name, const float* data);
    void setMat3(const std::string& name, const glm::mat3& mat);
    void setMat4(const std::string& name, const float* data);
    void setMat4(const std::string& name, const glm::mat4 &mat);

    GLuint getID() const { return program; }
};
