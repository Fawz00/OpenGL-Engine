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

class Mesh {
public:
    // constructor
    Mesh(std::vector<Vertex> vertices,
        std::vector<SubMesh> subMeshes,
        std::vector<Texture2D*> textures);
    ~Mesh();

    void Draw(Shader& shader);

private:
    unsigned int VAO, VBO;
    std::vector<Vertex> vertices;
    std::vector<SubMesh> subMeshes;
    std::vector<Texture2D*> textures;

    // initializes all the buffer objects/arrays
    void setupMesh();
};
