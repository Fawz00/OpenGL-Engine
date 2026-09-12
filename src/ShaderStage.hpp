#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
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
    std::string preprocessFile(
        const std::filesystem::path& filePath,
        uint32_t sourceId
    ) const;

    std::string processIncludes(
        const std::string& source,
        const std::filesystem::path& currentFile,
        uint32_t sourceId
    ) const;

    void buildDefines(std::ostringstream& stream) const;

    static std::string readTextFile(const std::string& filePath);

    mutable std::unordered_map<uint32_t, std::string> sourceFiles;
    mutable std::unordered_set<std::string> includeStack;

    uint32_t getSourceId(const std::filesystem::path& filePath) const;
};