#include "Shader.hpp"

Shader::~Shader() {
    if (program != 0) {
        glDeleteProgram(program);
    }
}

Shader& Shader::attachShader(ShaderType type, const std::string& path, std::initializer_list<std::string> defines) {
	if (linked) {
		throw std::runtime_error("Cannot attach shader after program has been linked.");
	}

	ShaderStage stage(type, path);
	for (auto& d : defines)
		stage.define(d);
	stages.push_back(stage);
	return *this;
}

Shader& Shader::define(const std::string& macro) {
    if (linked)
    {
        throw std::runtime_error("Cannot define macro after program has been linked.");
    }

    globalDefines.push_back(macro);
    return *this;
}

Shader& Shader::link() {
	std::vector<GLint> compiledShaderIds;

    for (auto& stage : stages) {
		// Add global defines to each stage
        for (auto& gd : globalDefines)
            stage.define(gd);

		GLint shader = stage.compile();

		compiledShaderIds.push_back(shader);
    }

    program = glCreateProgram();
    for (auto& shaderId : compiledShaderIds) {
        glAttachShader(program, shaderId);
	}
    glLinkProgram(program);

	// Check for linking errors
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        throw std::runtime_error("Program linking failed: " + std::string(infoLog));
    }

	// Cleanup compiled shaders
    for (auto& shaderId : compiledShaderIds) {
        glDetachShader(program, shaderId);
        glDeleteShader(shaderId);
	}

    linked = true;
    return *this;
}

void Shader::bind() {
    if (!linked) {
		Debug::logError("Attempted to bind unlinked shader program.");
        return;
    }
    glUseProgram(program);
}

void Shader::unbind() {
    glUseProgram(0);
}

GLint Shader::genAttrId(const std::string& name) {
    return glGetAttribLocation(program, name.c_str());
}

void Shader::setAttr(GLint id, GLint size, GLsizei stride, const void* offset, GLenum type) {
    glEnableVertexAttribArray(id);
    if (type == GL_INT) {
        glVertexAttribIPointer(id, size, type, stride, offset);
        return;
	}
    glVertexAttribPointer(id, size, type, GL_FALSE, stride, offset);
}

void Shader::disableAttr(GLint id) {
    glDisableVertexAttribArray(id);
}

GLint Shader::getCachedUniformLocation(std::unordered_map<std::string, GLint> cache, GLuint program, const std::string& name)
{
    auto it = cache.find(name);
    if (it != cache.end()) {
        return it->second;
    }
    GLint loc = glGetUniformLocation(static_cast<GLint>(program), name.c_str());
    cache.emplace(name, loc);
    return loc;
}

// Uniform setters
void Shader::setUInt(const std::string& name, unsigned int value) {
    GLint loc = getCachedUniformLocation(uniformLocations, program, name);
    glUniform1ui(loc, value);
}

void Shader::setInt(const std::string& name, int value) {
    GLint loc = getCachedUniformLocation(uniformLocations, program, name);
    glUniform1i(loc, value);
}

void Shader::setFloat(const std::string& name, float value) {
    GLint loc = getCachedUniformLocation(uniformLocations, program, name);
    glUniform1f(loc, value);
}

void Shader::setVec2(const std::string& name, const float* data) {
    GLint loc = getCachedUniformLocation(uniformLocations, program, name);
    glUniform2fv(loc, 1, data);
}
void Shader::setVec2(const std::string& name, const glm::vec2 &data) {
    GLint loc = getCachedUniformLocation(uniformLocations, program, name);
    glUniform2fv(loc, 1, glm::value_ptr(data));
}

void Shader::setVec3(const std::string& name, const float* data) {
    GLint loc = getCachedUniformLocation(uniformLocations, program, name);
    glUniform3fv(loc, 1, data);
}
void Shader::setVec3(const std::string& name, const glm::vec3& data) {
    GLint loc = getCachedUniformLocation(uniformLocations, program, name);
    glUniform3fv(loc, 1, glm::value_ptr(data));
}

void Shader::setVec4(const std::string& name, const float* data) {
    GLint loc = getCachedUniformLocation(uniformLocations, program, name);
    glUniform4fv(loc, 1, data);
}
void Shader::setVec4(const std::string& name, const glm::vec4& data) {
    GLint loc = getCachedUniformLocation(uniformLocations, program, name);
    glUniform4fv(loc, 1, glm::value_ptr(data));
}

void Shader::setMat4(const std::string& name, const float* data) {
    GLint loc = getCachedUniformLocation(uniformLocations, program, name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, data);
}
void Shader::setMat4(const std::string& name, const glm::mat4& mat) {
    GLint loc = getCachedUniformLocation(uniformLocations, program, name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
}