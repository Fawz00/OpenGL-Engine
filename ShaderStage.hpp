#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <glad/glad.h>

#include "Debug.hpp"

enum class ShaderType {
    VertexShader,
    FragmentShader,
    GeometryShader,
    ComputeShader
};

class ShaderStage {
public:
    ShaderStage(ShaderType type, const std::string& path)
        : type(type), path(path) {
    }
	~ShaderStage() = default;

    ShaderStage& define(const std::string& macro);

    std::string getPath() const;
    ShaderType getType() const;
    std::vector<std::string> getDefines() const;

    GLint compile() const;

private:
    ShaderType type;
    std::string path;
    std::vector<std::string> defines;

    std::string preprocessSource(const std::string& source) const;

    static std::string readTextFile(const std::string& filePath);
};
