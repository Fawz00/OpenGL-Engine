#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Material.hpp"
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
    unsigned int indexCount = 0; // filled by Mesh (indices are released after GPU upload)
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

/**
 * Mesh: GPU geometry (one interleaved VBO + one EBO per sub-mesh).
 *
 * Upload model
 * ------------
 * The setters (setPosition, setNormals, ...) only store CPU data and mark the mesh dirty.
 * upload() builds the interleaved VBO and sends it to the GPU ONCE. If upload() was
 * never called, the first draw does it automatically (keeps old call sites working).
 * upload(true) additionally frees the CPU copies of the attributes; after that the
 * attribute setters throw. Index data is always released right after the EBO upload.
 */
class Mesh {
public:
    Mesh(
        size_t vertexCount,
        std::vector<SubMesh> subMeshes,
        const Material* material = nullptr
    );
    ~Mesh();

    // No copy
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Move allowed
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    /** Binds the material (if any), draws the geometry and restores texture state. */
    void draw(Shader& shader);

    /**
     * Draws the geometry only. The caller is responsible for the material/texture state.
     * Used by Model::draw to avoid rebinding the same material for consecutive meshes.
     */
    void drawGeometry();

    /**
     * Builds the interleaved VBO from the stored attributes and uploads it (only if something changed).
     * @param releaseCpuData free the CPU copy of the attribute data after uploading.
     */
    void upload(bool releaseCpuData = false);

    [[nodiscard]] const Material* getMaterial() const noexcept { return m_Material; }
    void setMaterial(const Material* material) noexcept { m_Material = material; }

    // Setters for vertex attributes (store data, upload happens in upload())
    void setPosition(std::vector<glm::vec3> positions);
    void setNormals(std::vector<glm::vec3> normals);
    void setTexCoords(std::uint8_t index, std::vector<glm::vec2> texCoords);
    void setTangents(std::vector<glm::vec3> tangents);
    void setBitangents(std::vector<glm::vec3> bitangents);

    /** Bone slots per vertex. Must be uploaded as integers (GL_INT -> glVertexAttribIPointer). */
    void setBoneIDs(std::vector<std::array<int, MAX_BONE_INFLUENCE>> boneIDs);
    void setWeights(std::vector<std::array<float, MAX_BONE_INFLUENCE>> weights);

private:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    size_t vertexCount = 0;
    std::vector<VertexAttribute> attributes;
    std::vector<SubMesh> subMeshes;
    const Material* m_Material = nullptr;

    bool m_Dirty = false;
    bool m_CpuDataReleased = false;

    // creates VAO/VBO/EBOs
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
