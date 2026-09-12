#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

#include "Shader.hpp"
#include "Texture2D.hpp"

#define MAX_BONE_INFLUENCE 4

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;

    //bone indexes which will influence this vertex
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    //weights from each bone
    float m_Weights[MAX_BONE_INFLUENCE];
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
    Mesh(std::vector<Vertex> vertices,
        std::vector<SubMesh> subMeshes,
        std::vector<Texture2D*> textures);
    ~Mesh();

    // No copy
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Move allowed
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

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
    std::vector<Vertex> vertices;
    std::vector<SubMesh> subMeshes;
    std::vector<Texture2D*> textures;

    // initializes all the buffer objects/arrays
    void setupMesh();
};
