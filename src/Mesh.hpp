#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

#include "Shader.hpp"
#include "Texture2D.hpp"

#define MAX_BONE_INFLUENCE 4

struct VertexAttribute {
    GLuint location;
    GLint componentCount;
    GLenum type;
    GLboolean normalized;

    size_t elementSize;
    std::vector<std::byte> data;
};

struct SubMesh {
    std::vector<unsigned int> indices;
    GLenum primitiveType;
    unsigned int EBO = 0;
};

enum Texture2DFlags : uint32_t {
    HasDiffuse = 1 << 0,
    HasSpecular = 1 << 1,
    HasNormal = 1 << 2,
    HasHeight = 1 << 3,
};

enum VertexAttributeId : uint32_t {
    Position = 0,
    Normal = 1,
    TexCoords0 = 2,
    Tangent = 3,
    Bitangent = 4,
    BoneIDs = 5,
    Weights = 6,
};

class Mesh {
public:
    // constructor
    Mesh(
        size_t vertexCount,
        std::vector<SubMesh> subMeshes,
        std::vector<Texture2D*> textures
    );
    ~Mesh();

    // No copy
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Move allowed
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void draw(Shader& shader);

	// Setters for vertex attributes
    void setPosition(std::vector<glm::vec3> positions);
	void setNormals(std::vector<glm::vec3> normals);
	void setTexCoords(std::uint8_t index, std::vector<glm::vec2> texCoords);
	void setTangents(std::vector<glm::vec3> tangents);
	void setBitangents(std::vector<glm::vec3> bitangents);

	void setBoneIDs(std::vector<std::array<int, MAX_BONE_INFLUENCE>> boneIDs);
	void setWeights(std::vector<std::array<float, MAX_BONE_INFLUENCE>> weights);

private:
    unsigned int VAO, VBO;
    size_t vertexCount;
    std::vector<VertexAttribute> attributes;
    std::vector<SubMesh> subMeshes;
    std::vector<Texture2D*> textures;

    // initializes all the buffer objects/arrays
    void setupMesh();

    // Rebuilds interleaved VBO from active attributes
    void rebuildVertexBuffer();

    void setAttribute(
        GLuint location,
        GLint componentCount,
        GLenum type,
        GLboolean normalized,
        size_t elementSize,
        std::vector<std::byte> data
    );

    VertexAttribute* findAttribute(GLuint location);
};
