#include "ShaderStage.hpp"

#include "Mesh.hpp"

ShaderStage& ShaderStage::define(const std::string& macro) {
    defines.push_back(macro);
    return *this;
}

std::string ShaderStage::readTextFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filePath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string ShaderStage::preprocessSource(const std::string& source) const {
    std::istringstream in(source);
    std::ostringstream out;

    std::string line;
    bool versionFound = false;
    int lineCount = 0;

    while (std::getline(in, line)) {
        ++lineCount;
        out << line << '\n';

		// Detect #version directive
        if (!versionFound && line.find("#version") != std::string::npos) {
            versionFound = true;

			// Add user-defined macros after #version
			buildDefines(out);

			// Reset line numbering
            out << "#line " << (lineCount + 1) << '\n';
        }
    }

	// If no #version directive was found, add one at the top
    if (!versionFound) {
        std::ostringstream final;
        final << "#version 330\n"; // default fallback
		buildDefines(final);
        final << "#line 1\n";
        final << out.str();
        return final.str();
    }

    return out.str();
}

void ShaderStage::buildDefines(std::ostringstream& stream) const {
    for (const auto& def : defines) {
        stream << "#define " << def << '\n';
    }

    if (type == ShaderType::VertexShader) {
        stream << "#define VERTEX_SHADER\n";
    } else if (type == ShaderType::FragmentShader) {
        stream << "#define FRAGMENT_SHADER\n";
    } else if (type == ShaderType::GeometryShader) {
        stream << "#define GEOMETRY_SHADER\n";
    } else if (type == ShaderType::ComputeShader) {
        stream << "#define COMPUTE_SHADER\n";
	}
}

GLint ShaderStage::compile() const {
	// Type to GLenum mapping
	GLenum glType{};
	switch (type) {
        case ShaderType::VertexShader:
            glType = GL_VERTEX_SHADER;
		    break;
        case ShaderType::FragmentShader:
			glType = GL_FRAGMENT_SHADER;
            break;
		case ShaderType::GeometryShader:
			glType = GL_GEOMETRY_SHADER;
			break;
		case ShaderType::ComputeShader:
			throw std::runtime_error("Unimplemented shader stage type: ComputeShader");
			break;
        default:
			Debug::logError("Unknown shader stage type.");
            throw std::runtime_error("Unknown shader stage type.");
	}

    GLuint shader = glCreateShader(glType);
	std::string sourceRaw = readTextFile(path);
    std::string source = preprocessSource(sourceRaw);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        Debug::logError("Shader compilation error:\n" + std::string(infoLog) + "\nShader source path: " + path);
		Debug::logError("Shader source:\n" + sourceRaw);
        throw std::runtime_error("Shader compilation failed: " + std::string(infoLog));
    }
    return shader;
}

std::string ShaderStage::getPath() const
{
    return path;
}

ShaderType ShaderStage::getType() const
{
    return type;
}
std::vector<std::string> ShaderStage::getDefines() const
{
    return defines;
}
