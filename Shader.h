#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Shader {
private:
    GLuint program;

    static std::string loadShaderSource(const std::string& filePath);
    GLuint compileShader(const std::string& source, GLenum type);

public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    void use();
    void stop();
    void destroy();

    GLint genAttrId(const std::string& name);
    void setAttr(GLint id, GLint size, GLsizei stride, const void* offset);
    void disableAttr(GLint id);
    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void setVec2(const std::string& name, const float* data);
    void setVec3(const std::string& name, const float* data);
    void setVec4(const std::string& name, const float* data);
    void setMat4(const std::string& name, const float* data);

    GLuint getID() const { return program; }
};