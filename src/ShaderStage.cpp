#include "ShaderStage.hpp"

#include <cstdint>
#include <regex>

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

uint32_t ShaderStage::getSourceId(
    const std::filesystem::path& filePath
) const {
	// Check if the file path is already registered
    for (const auto& [id, registeredPath] : sourceFiles) {
        if (registeredPath == filePath.string()) {
            return id;
        }
    }

    uint32_t sourceId = static_cast<uint32_t>(sourceFiles.size());

    sourceFiles[sourceId] = filePath.string();

    return sourceId;
}

std::string ShaderStage::preprocessFile(
    const std::filesystem::path& filePath,
    uint32_t sourceId
) const {
    std::filesystem::path normalizedPath =
        std::filesystem::weakly_canonical(filePath);

    std::string normalizedString = normalizedPath.string();

	// Check for circular includes
    if (includeStack.contains(normalizedString)) {
        throw std::runtime_error(
            "Circular shader include detected: " +
            normalizedString
        );
    }

    includeStack.insert(normalizedString);

    sourceFiles[sourceId] = normalizedString;

    std::string source = readTextFile(normalizedString);

    std::ostringstream output;

	// Reset line numbering to the beginning of the included file
    output << "#line 1 " << sourceId << '\n';

    output << processIncludes(
        source,
        normalizedPath,
        sourceId
    );

    includeStack.erase(normalizedString);

    return output.str();
}

std::string ShaderStage::processIncludes(
    const std::string& source,
    const std::filesystem::path& currentFile,
    uint32_t sourceId
) const {
    std::istringstream in(source);
    std::ostringstream out;

    std::string line;
    int lineNumber = 0;

	// Regular expression to match #include directives
    static const std::regex includeRegex(
        R"(^\s*#\s*include\s*"([^"]+)\"\s * $)"
        );

        while (std::getline(in, line)) {
            ++lineNumber;

            std::smatch match;

            if (std::regex_match(line, match, includeRegex)) {
                std::string includeName = match[1].str();

				// Resolve path relative to current file
                std::filesystem::path includePath =
                    currentFile.parent_path() / includeName;

                includePath =
                    std::filesystem::weakly_canonical(includePath);

                if (!std::filesystem::exists(includePath)) {
                    throw std::runtime_error(
                        "Shader include file not found: " +
                        includePath.string() +
                        "\nIncluded from: " +
                        currentFile.string() +
                        ":" +
                        std::to_string(lineNumber)
                    );
                }

                uint32_t includeSourceId =
                    getSourceId(includePath);

				// Preprocess the included file and insert its content
                out << preprocessFile(
                    includePath,
                    includeSourceId
                );

				// After including the file, reset the line numbering to the current file
                out << "#line "
                    << (lineNumber + 1)
                    << " "
                    << sourceId
                    << '\n';

                continue;
            }

            out << line << '\n';
        }

        return out.str();
}

std::string ShaderStage::preprocessSource(
    const std::string& source
) const {
    sourceFiles.clear();
    includeStack.clear();

    std::filesystem::path mainPath =
        std::filesystem::weakly_canonical(path);

    sourceFiles[0] = mainPath.string();

    std::istringstream in(source);
    std::ostringstream out;

    std::string line;
    bool versionFound = false;
    int lineCount = 0;

    while (std::getline(in, line)) {
        ++lineCount;

        // Detect #version directive
        if (!versionFound &&
            line.find("#version") != std::string::npos) {

            versionFound = true;

            // Keep #version
            out << line << '\n';

            // Add user-defined macros after #version
            buildDefines(out);

            // Reset line numbering
            out << "#line "
                << (lineCount + 1)
                << " 0\n";

            continue;
        }

		// If #version has not been found yet, just copy the line
        if (!versionFound) {
            out << line << '\n';
            continue;
        }

		// Process #include directives
        std::smatch match;

        static const std::regex includeRegex(
            R"REGEX(^\s*#\s*include\s*"([^"]+)"\s*$)REGEX"
            );

            if (std::regex_match(line, match, includeRegex)) {
                std::string includeName = match[1].str();

                std::filesystem::path includePath =
                    mainPath.parent_path() / includeName;

                includePath =
                    std::filesystem::weakly_canonical(includePath);

                if (!std::filesystem::exists(includePath)) {
                    throw std::runtime_error(
                        "Shader include file not found: " +
                        includePath.string() +
                        "\nIncluded from: " +
                        mainPath.string() +
                        ":" +
                        std::to_string(lineCount)
                    );
                }

                uint32_t includeSourceId =
                    getSourceId(includePath);

                out << preprocessFile(
                    includePath,
                    includeSourceId
                );

				// After including the file, reset the line numbering to the current file
                out << "#line "
                    << (lineCount + 1)
                    << " 0\n";

                continue;
            }

            out << line << '\n';
    }

	// If no #version directive was found, prepend it along with user-defined macros
    if (!versionFound) {
        std::ostringstream final;

        final << "#version 330\n";

        buildDefines(final);

        final << "#line 1 0\n";

        final << processIncludes(
            source,
            mainPath,
            0
        );

        return final.str();
    }

    return out.str();
}

void ShaderStage::buildDefines(
    std::ostringstream& stream
) const {
    for (const auto& def : defines) {
        stream << "#define " << def << '\n';
    }

    if (type == ShaderType::VertexShader) {
        stream << "#define VERTEX_SHADER\n";
    }
    else if (type == ShaderType::FragmentShader) {
        stream << "#define FRAGMENT_SHADER\n";
    }
    else if (type == ShaderType::GeometryShader) {
        stream << "#define GEOMETRY_SHADER\n";
    }
    else if (type == ShaderType::ComputeShader) {
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
        throw std::runtime_error( "Unimplemented shader stage type: ComputeShader" );
        break;

    default:
        Debug::logError("Unknown shader stage type.");

        throw std::runtime_error( "Unknown shader stage type." );
    }

    GLuint shader = glCreateShader(glType);

    std::string sourceRaw = readTextFile(path);
    std::string source = preprocessSource(sourceRaw);

    const char* src = source.c_str();

    glShaderSource( shader, 1, &src, nullptr );
    glCompileShader(shader);

    GLint success;
    glGetShaderiv( shader, GL_COMPILE_STATUS, &success );

    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog( shader, 512, nullptr, infoLog );

        Debug::logError( "Shader compilation error:\n" + std::string(infoLog) + "\nShader source path: " + path );
        Debug::logError( "Shader source:\n" + sourceRaw );
        throw std::runtime_error( "Shader compilation failed: " + std::string(infoLog) );
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