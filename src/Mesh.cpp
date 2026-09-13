#include "Mesh.hpp"

#include <stdexcept>
#include <array>

Mesh::Mesh(
    size_t vertexCount,
    std::vector<SubMesh> subMeshes,
    std::vector<Texture2D*> textures
)
    : vertexCount(vertexCount),
    subMeshes(std::move(subMeshes)),
    textures(std::move(textures))
{
    setupMesh();
}

Mesh::Mesh(Mesh&& other) noexcept
    : VAO(other.VAO),
    VBO(other.VBO),
    vertexCount(other.vertexCount),
    attributes(std::move(other.attributes)),
    subMeshes(std::move(other.subMeshes)),
    textures(std::move(other.textures))
{
    other.VAO = 0;
    other.VBO = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        for (auto& sm : subMeshes) {
            if (sm.EBO) glDeleteBuffers(1, &sm.EBO);
        }

        VAO = other.VAO;
        VBO = other.VBO;
        vertexCount = other.vertexCount;
        attributes = std::move(other.attributes);
        subMeshes = std::move(other.subMeshes);
        textures = std::move(other.textures);

        other.VAO = 0;
        other.VBO = 0;
    }
    return *this;
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    for (auto& sm : subMeshes) {
        if (sm.EBO)
            glDeleteBuffers(1, &sm.EBO);
    }
}

void Mesh::draw(Shader& shader)
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
        Texture2D::Texture2DType texType = textures[i]->getType();

        if (texType == Texture2D::Texture2DType::TextureDiffuse) {
            name = "texture_diffuse";
            number = std::to_string(diffuseNr++);
            flags |= HasDiffuse;
        }
        else if (texType == Texture2D::Texture2DType::TextureSpecular) {
            name = "texture_specular";
            number = std::to_string(specularNr++);
			flags |= HasSpecular;
        }
        else if (texType == Texture2D::Texture2DType::TextureNormal) {
            name = "texture_normal";
            number = std::to_string(normalNr++);
			flags |= HasNormal;
        }
        else if (texType == Texture2D::Texture2DType::TextureHeight) {
            name = "texture_height";
            number = std::to_string(heightNr++);
			flags |= HasHeight;
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
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

	// always good practice to set everything back to defaults once configured.
    glActiveTexture(GL_TEXTURE0);
    Texture2D::unbind();
}

VertexAttribute* Mesh::findAttribute(GLuint location)
{
    for (auto& attribute : attributes) {
        if (attribute.location == location)
            return &attribute;
    }

    return nullptr;
}

void Mesh::setAttribute(
    GLuint location,
    GLint componentCount,
    GLenum type,
    GLboolean normalized,
    size_t elementSize,
    std::vector<std::byte> data
)
{
    if (data.size() != vertexCount * elementSize) {
        throw std::runtime_error(
            "Vertex attribute data size does not match vertex count."
        );
    }

    VertexAttribute attribute{
        location,
        componentCount,
        type,
        normalized,
        elementSize,
        std::move(data)
    };

    if (auto* existing = findAttribute(location)) {
        *existing = std::move(attribute);
    }
    else {
        attributes.push_back(std::move(attribute));
    }

    rebuildVertexBuffer();
}

// SETTER
// Position
void Mesh::setPosition(std::vector<glm::vec3> positions)
{
    std::vector<std::byte> data(
        positions.size() * sizeof(glm::vec3)
    );

    std::memcpy(
        data.data(),
        positions.data(),
        data.size()
    );

    setAttribute(
        VertexAttributeId::Position,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(glm::vec3),
        std::move(data)
    );
}

// Normal
void Mesh::setNormals(std::vector<glm::vec3> normals)
{
    std::vector<std::byte> data(
        normals.size() * sizeof(glm::vec3)
    );

    std::memcpy(
        data.data(),
        normals.data(),
        data.size()
    );

    setAttribute(
        VertexAttributeId::Normal,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(glm::vec3),
        std::move(data)
    );
}

// UV
void Mesh::setTexCoords(
    std::uint8_t index,
    std::vector<glm::vec2> texCoords
)
{
    std::vector<std::byte> data(
        texCoords.size() * sizeof(glm::vec2)
    );

    std::memcpy(
        data.data(),
        texCoords.data(),
        data.size()
    );

    GLuint location = VertexAttributeId::TexCoords0 + index;

    setAttribute(
        location,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(glm::vec2),
        std::move(data)
    );
}

// Tangent
void Mesh::setTangents(std::vector<glm::vec3> tangents)
{
    std::vector<std::byte> data(
        tangents.size() * sizeof(glm::vec3)
    );

    std::memcpy(
        data.data(),
        tangents.data(),
        data.size()
    );

    setAttribute(
        VertexAttributeId::Tangent,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(glm::vec3),
        std::move(data)
    );
}

// Bitangent
void Mesh::setBitangents(std::vector<glm::vec3> bitangents)
{
    std::vector<std::byte> data(
        bitangents.size() * sizeof(glm::vec3)
    );

    std::memcpy(
        data.data(),
        bitangents.data(),
        data.size()
    );

    setAttribute(
        VertexAttributeId::Bitangent,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(glm::vec3),
        std::move(data)
    );
}

// Bone IDs
void Mesh::setBoneIDs(
    std::vector<std::array<int, MAX_BONE_INFLUENCE>> boneIDs
)
{
    std::vector<std::byte> data(
        boneIDs.size() * sizeof(boneIDs[0])
    );

    std::memcpy(
        data.data(),
        boneIDs.data(),
        data.size()
    );

    setAttribute(
        VertexAttributeId::BoneIDs,
        4,
        GL_INT,
        GL_FALSE,
        sizeof(boneIDs[0]),
        std::move(data)
    );
}

// Weights
void Mesh::setWeights(
    std::vector<std::array<float, MAX_BONE_INFLUENCE>> weights
)
{
    std::vector<std::byte> data(
        weights.size() * sizeof(weights[0])
    );

    std::memcpy(
        data.data(),
        weights.data(),
        data.size()
    );

    setAttribute(
        VertexAttributeId::Weights,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(weights[0]),
        std::move(data)
    );
}

void Mesh::setupMesh()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Initially empty.
    glBufferData(
        GL_ARRAY_BUFFER,
        0,
        nullptr,
        GL_STATIC_DRAW
    );

    // Create EBO for each sub-mesh
    for (auto& sm : subMeshes) {
        glGenBuffers(1, &sm.EBO);

        glBindBuffer(
            GL_ELEMENT_ARRAY_BUFFER,
            sm.EBO
        );

        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            sm.indices.size() * sizeof(unsigned int),
            sm.indices.data(),
            GL_STATIC_DRAW
        );
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

void Mesh::rebuildVertexBuffer()
{
    if (attributes.empty() || vertexCount == 0)
        return;

    // Calculate stride
    size_t stride = 0;

    for (const auto& attribute : attributes)
        stride += attribute.elementSize;

    // Create interleaved buffer
    std::vector<std::byte> buffer(
        vertexCount * stride
    );

    for (size_t vertex = 0; vertex < vertexCount; ++vertex)
    {
        size_t offset = 0;

        for (const auto& attribute : attributes)
        {
            std::memcpy(
                buffer.data()
                + vertex * stride
                + offset,

                attribute.data.data()
                + vertex * attribute.elementSize,

                attribute.elementSize
            );

            offset += attribute.elementSize;
        }
    }

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        buffer.size(),
        buffer.data(),
        GL_STATIC_DRAW
    );


    // Configure attributes through Shader class
    size_t offset = 0;

    for (const auto& attribute : attributes)
    {
        Shader::setAttr(
            attribute.location,
            attribute.componentCount,
            stride,
            reinterpret_cast<void*>(offset),
            attribute.type,
            attribute.normalized
        );

        offset += attribute.elementSize;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}