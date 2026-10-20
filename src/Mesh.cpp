#include "Mesh.hpp"

#include <cstring>
#include <memory>
#include <stdexcept>

namespace {
    template <typename T>
    std::vector<std::byte> toBytes(const std::vector<T>& values)
    {
        std::vector<std::byte> data(values.size() * sizeof(T));
        if (!data.empty()) {
            std::memcpy(data.data(), values.data(), data.size());
        }
        return data;
    }
}

Mesh::Mesh(
    size_t vertexCount,
    std::vector<SubMesh> subMeshes,
    const Material* material
)
    : vertexCount(vertexCount),
    subMeshes(std::move(subMeshes)),
    m_Material(material)
{
    setupMesh();
}

Mesh::Mesh(Mesh&& other) noexcept
    : VAO(other.VAO),
    VBO(other.VBO),
    vertexCount(other.vertexCount),
    attributes(std::move(other.attributes)),
    subMeshes(std::move(other.subMeshes)),
    m_Material(other.m_Material),
    m_Dirty(other.m_Dirty),
    m_CpuDataReleased(other.m_CpuDataReleased)
{
    other.VAO = 0;
    other.VBO = 0;
    other.subMeshes.clear();
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
        m_Material = other.m_Material;
        m_Dirty = other.m_Dirty;
        m_CpuDataReleased = other.m_CpuDataReleased;

        other.VAO = 0;
        other.VBO = 0;
        other.subMeshes.clear();
    }
    return *this;
}

Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    for (auto& sm : subMeshes) {
        if (sm.EBO)
            glDeleteBuffers(1, &sm.EBO);
    }
}

void Mesh::draw(Shader& shader)
{
    if (m_Material) {
        m_Material->bind(shader);
    }
    else {
        Material::bindNone(shader);
    }

    drawGeometry();

    // always good practice to set everything back to defaults once configured.
    Material::unbindAll();
}

void Mesh::drawGeometry()
{
    if (m_Dirty) {
        upload(false);
    }

    glBindVertexArray(VAO);
    for (const auto& sm : subMeshes) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sm.EBO);
        glDrawElements(sm.primitiveType, static_cast<GLsizei>(sm.indexCount), GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
}

void Mesh::upload(bool releaseCpuData)
{
    if (m_Dirty) {
        rebuildVertexBuffer();
        m_Dirty = false;
    }

    if (releaseCpuData && !m_CpuDataReleased) {
        for (auto& attribute : attributes) {
            std::vector<std::byte>().swap(attribute.data);
        }
        m_CpuDataReleased = true;
    }
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
    if (m_CpuDataReleased) {
        throw std::logic_error(
            "Mesh: CPU attribute data was released by upload(true); attributes can no longer be modified."
        );
    }

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

    m_Dirty = true;
}

// SETTERS ------------------------------------------------------------------

void Mesh::setPosition(std::vector<glm::vec3> positions)
{
    setAttribute(VertexAttributeId::Position, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), toBytes(positions));
}

void Mesh::setNormals(std::vector<glm::vec3> normals)
{
    setAttribute(VertexAttributeId::Normal, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), toBytes(normals));
}

void Mesh::setTexCoords(std::uint8_t index, std::vector<glm::vec2> texCoords)
{
    setAttribute(
        VertexAttributeId::TexCoords0 + index,
        2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), toBytes(texCoords)
    );
}

void Mesh::setTangents(std::vector<glm::vec3> tangents)
{
    setAttribute(VertexAttributeId::Tangent, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), toBytes(tangents));
}

void Mesh::setBitangents(std::vector<glm::vec3> bitangents)
{
    setAttribute(VertexAttributeId::Bitangent, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), toBytes(bitangents));
}

void Mesh::setBoneIDs(std::vector<std::array<int, MAX_BONE_INFLUENCE>> boneIDs)
{
    setAttribute(
        VertexAttributeId::BoneIDs,
        MAX_BONE_INFLUENCE, GL_INT, GL_FALSE, sizeof(std::array<int, MAX_BONE_INFLUENCE>), toBytes(boneIDs)
    );
}

void Mesh::setWeights(std::vector<std::array<float, MAX_BONE_INFLUENCE>> weights)
{
    setAttribute(
        VertexAttributeId::Weights,
        MAX_BONE_INFLUENCE, GL_FLOAT, GL_FALSE, sizeof(std::array<float, MAX_BONE_INFLUENCE>), toBytes(weights)
    );
}

// GPU SETUP ----------------------------------------------------------------

void Mesh::setupMesh()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    // Create EBO for each sub-mesh, upload indices, then free the CPU copy:
    // indices are never needed again (draw only needs the count).
    for (auto& sm : subMeshes) {
        sm.indexCount = static_cast<unsigned int>(sm.indices.size());

        glGenBuffers(1, &sm.EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sm.EBO);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(sm.indices.size() * sizeof(unsigned int)),
            sm.indices.data(),
            GL_STATIC_DRAW
        );

        std::vector<unsigned int>().swap(sm.indices);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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

    // Interleave attribute by attribute: the source is read sequentially and no
    // zero-initialisation pass is paid for the temporary buffer.
    const size_t totalSize = vertexCount * stride;
    auto buffer = std::make_unique_for_overwrite<std::byte[]>(totalSize);

    size_t offset = 0;
    for (const auto& attribute : attributes) {
        const std::byte* src = attribute.data.data();
        std::byte* dst = buffer.get() + offset;
        const size_t elementSize = attribute.elementSize;

        for (size_t v = 0; v < vertexCount; ++v) {
            std::memcpy(dst + v * stride, src + v * elementSize, elementSize);
        }
        offset += elementSize;
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(totalSize), buffer.get(), GL_STATIC_DRAW);

    // Configure attributes through Shader class (enabled state is stored in the VAO)
    offset = 0;
    for (const auto& attribute : attributes) {
        Shader::setAttr(
            attribute.location,
            attribute.componentCount,
            static_cast<GLsizei>(stride),
            reinterpret_cast<void*>(offset),
            attribute.type,
            attribute.normalized
        );
        offset += attribute.elementSize;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    // NOTE: the old code called Shader::disableAttr() here, AFTER unbinding the VAO. That ran on
    // VAO 0 (GL_INVALID_OPERATION in Core profile) and did not affect this mesh, so it was removed.
}
