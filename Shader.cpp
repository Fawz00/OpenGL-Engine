#include "Shader.h"
#include "Debug.h"

std::string Shader::loadShaderSource(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filePath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint Shader::compileShader(const std::string& source, GLenum type) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		Debug::logError("Shader compilation error: " + std::string(infoLog));
        throw std::runtime_error("Shader compilation failed: " + std::string(infoLog));
    }
    return shader;
}

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexCode = loadShaderSource(vertexPath);
    std::string fragmentCode = loadShaderSource(fragmentPath);

    GLuint vertexShader = compileShader(vertexCode, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentCode, GL_FRAGMENT_SHADER);

    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        throw std::runtime_error("Program linking failed: " + std::string(infoLog));
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Shader::use() {
    glUseProgram(program);
}

void Shader::stop() {
    glUseProgram(0);
}

void Shader::destroy() {
    if (program != 0) {
        glDeleteProgram(program);
        program = 0;
    }
}

GLint Shader::genAttrId(const std::string& name) {
    return glGetAttribLocation(program, name.c_str());
}

void Shader::setAttr(GLint id, GLint size, GLsizei stride, const void* offset) {
    glVertexAttribPointer(id, size, GL_FLOAT, GL_FALSE, stride, offset);
    glEnableVertexAttribArray(id);
}

void Shader::disableAttr(GLint id) {
    glDisableVertexAttribArray(id);
}

// Uniform setters
void Shader::setInt(const std::string& name, int value) {
    glUniform1i(glGetUniformLocation(program, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) {
    glUniform1f(glGetUniformLocation(program, name.c_str()), value);
}

void Shader::setVec2(const std::string& name, const float* data) {
    glUniform2fv(glGetUniformLocation(program, name.c_str()), 1, data);
}

void Shader::setVec3(const std::string& name, const float* data) {
    glUniform3fv(glGetUniformLocation(program, name.c_str()), 1, data);
}

void Shader::setVec4(const std::string& name, const float* data) {
    glUniform4fv(glGetUniformLocation(program, name.c_str()), 1, data);
}

void Shader::setMat4(const std::string& name, const float* data) {
    glUniformMatrix4fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE, data);
}