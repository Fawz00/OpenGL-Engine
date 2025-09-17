#include "Mesh.hpp"

Mesh::Mesh(std::vector<Vertex> vertices,
    std::vector<SubMesh> subMeshes,
    std::vector<Texture2D*> textures)
    : vertices(std::move(vertices)),
    subMeshes(std::move(subMeshes)),
    textures(std::move(textures))
{
    setupMesh();
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    for (auto& sm : subMeshes) {
        if (sm.EBO) glDeleteBuffers(1, &sm.EBO);
    }
}

void Mesh::Draw(Shader& shader)
{
    uint32_t flags = 0;

    // bind appropriate textures
	unsigned int otherNr = 1;
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr = 1;
    unsigned int heightNr = 1;

    for (unsigned int i = 0; i < textures.size(); i++) {
        std::string number;
        std::string name;
        Texture2D::TextureType texType = textures[i]->getType();

        if (texType == Texture2D::TextureType::TextureDiffuse) {
            name = "texture_diffuse";
            number = std::to_string(diffuseNr++);
            flags |= Texture2D::HasDiffuse;
        }
        else if (texType == Texture2D::TextureType::TextureSpecular) {
            name = "texture_specular";
            number = std::to_string(specularNr++);
			flags |= Texture2D::HasSpecular;
        }
        else if (texType == Texture2D::TextureType::TextureNormal) {
            name = "texture_normal";
            number = std::to_string(normalNr++);
			flags |= Texture2D::HasNormal;
        }
        else if (texType == Texture2D::TextureType::TextureHeight) {
            name = "texture_height";
            number = std::to_string(heightNr++);
			flags |= Texture2D::HasHeight;
        }
        else {
            name = "texture_other";
            number = std::to_string(otherNr++);
        }

        shader.setInt(name + number, i);
        textures[i]->bind(i);
    }

	// Texture bitmask for shader
    shader.setUInt("uTextureFlags", flags);

    // draw mesh
    glBindVertexArray(VAO);
    for (auto& sm : subMeshes) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sm.EBO);
        glDrawElements(sm.primitiveType,
                       static_cast<unsigned int>(sm.indices.size()),
                       GL_UNSIGNED_INT,
                       0);
    }
    glBindVertexArray(0);

	// always good practice to set everything back to defaults once configured.
    glActiveTexture(GL_TEXTURE0);
    Texture2D::unbind();
}

void Mesh::setupMesh()
{
    // VAO + VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_STATIC_DRAW);

	// Create EBO for each sub-mesh
    for (auto& sm : subMeshes) {
        glGenBuffers(1, &sm.EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sm.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            sm.indices.size() * sizeof(unsigned int),
            sm.indices.data(),
            GL_STATIC_DRAW);
    }

    // set vertex attributes
    Shader::setAttr(0, 3, sizeof(Vertex), (void*)0);
    Shader::setAttr(1, 3, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    Shader::setAttr(2, 2, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    Shader::setAttr(3, 3, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
    Shader::setAttr(4, 3, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
    Shader::setAttr(5, 4, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs), GL_INT);
    Shader::setAttr(6, 4, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}