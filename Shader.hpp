#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Debug.hpp"

class Shader {
private:
    GLuint program;

    static std::string loadShaderSource(const std::string& filePath);
    static GLuint compileShader(const std::string& source, const std::string& path, GLenum type);

public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    void use();
    static void stop();
    ~Shader();

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