#pragma once

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <unordered_map>

#include "stb_image.h"
#include "Mesh.hpp"
#include "Material.hpp"
#include "Rig.hpp"
#include "Shader.hpp"
#include "Texture2D.hpp"
#include "AssimpGLMHelpers.hpp"
#include "Debug.hpp"

/**
 * Model: loader of the original model data. READ-ONLY after construction.
 *
 * Owns:
 *  - Mesh      (GPU geometry, with bone ids/weights as vertex attributes when rigged)
 *  - Texture2D (shared, loaded once per path)
 *  - Material  (texture slots only, for now; one per scene material, shared by meshes)
 *  - Rig       (skeleton: hierarchy, bind pose, bone offsets)
 *
 * It has no runtime/animation state: animating is done by Animator, which only reads
 * the Rig. The only non-const operation is draw() on GL state, which is const here.
 */
class Model
{
public:
    enum ConfigOption {
        Never,
        IfAvailable,
        Force
    };

    /**
     * How Assimp generates normals for meshes that have none (only used when normals are not ConfigOption::Never).
     * Flat   : one normal per face (hard edges). Default.
     * Smooth : normals averaged across shared vertices (soft shading).
     * Assimp does not allow both at once (the two post-process flags are incompatible), so exactly one is used.
     */
    enum class NormalMode {
        Flat,
        Smooth
    };

    class ImportConfig {
    public:
        ImportConfig() = default;

        // Setters for each configuration option
        ImportConfig& setPositions(ConfigOption option) { positions = option; return *this; }
        ImportConfig& setTextures(ConfigOption option) { textures = option; return *this; }
        ImportConfig& setNormals(ConfigOption option) { normals = option; return *this; }
        ImportConfig& setNormalMode(NormalMode mode) { normalMode = mode; return *this; }
        ImportConfig& setTangents(ConfigOption option) { tangents = option; return *this; }
        ImportConfig& setRig(ConfigOption option) { rig = option; return *this; }
        ImportConfig& setTriangulation(ConfigOption option) { triangulation = option; return *this; }
        ImportConfig& setLines(ConfigOption option) { lines = option; return *this; }
        ImportConfig& setPoints(ConfigOption option) { points = option; return *this; }

        ImportConfig& setGammaCorrection(bool enable) { gammaCorrection = enable; return *this; }

        // Getters for each configuration option
        ConfigOption getPositions() const { return positions; }
        ConfigOption getTextures() const { return textures; }
        ConfigOption getNormals() const { return normals; }
        NormalMode getNormalMode() const { return normalMode; }
        ConfigOption getTangents() const { return tangents; }
        ConfigOption getRig() const { return rig; }
        ConfigOption getTriangulation() const { return triangulation; }
        ConfigOption getLines() const { return lines; }
        ConfigOption getPoints() const { return points; }

        bool isGammaCorrectionEnabled() const { return gammaCorrection; }
    private:
        ConfigOption positions = IfAvailable;
        ConfigOption textures = IfAvailable;
        ConfigOption normals = IfAvailable;
        NormalMode normalMode = NormalMode::Flat;
        ConfigOption tangents = IfAvailable;
        ConfigOption rig = IfAvailable;
        ConfigOption triangulation = IfAvailable;
        ConfigOption lines = IfAvailable;
        ConfigOption points = IfAvailable;

        bool gammaCorrection = false;
    };

    enum ModelFeature : uint32_t
    {
        None = 0,
        Positions = 1 << 0,
        Textures = 1 << 1,
        Normals = 1 << 2,
        Tangents = 1 << 3,
        Rig = 1 << 4,
        Animation = 1 << 5,
    };

    // constructor, expects a filepath to a 3D model.
    // (Two overloads instead of a default argument: GCC/Clang reject `ImportConfig config = ImportConfig()`
    //  for a nested class with default member initializers. Call sites are unchanged.)
    Model(std::string const& path);
    Model(std::string const& path, ImportConfig config);
    ~Model() = default;

    // No copy
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    // Move allowed
    Model(Model&&) noexcept = default;
    Model& operator=(Model&&) noexcept = default;

    /** Draws all meshes. Consecutive meshes that share a material do not rebind textures. */
    void draw(Shader& shader) const;

    // ---- Read-only getters ------------------------------------------------
    /** True when the file was imported successfully. */
    bool isValid() const noexcept { return m_Loaded; }
    /** Directory of the model file (name kept from the previous API). */
    const std::string& getPath() const { return directory; }
    ModelFeature getModelFeature() const { return modelFeature; }

    /** Skeleton data. Empty (Rig::empty()) when the model has no bones. */
    const ::Rig& getRig() const noexcept { return *m_Rig; }
    bool hasRig() const noexcept { return !m_Rig->empty(); }
    /**
     * Shared handle to the rig. Animator keeps one, so a bound Animator stays valid
     * even if this Model object is moved (e.g. stored in a std::vector).
     */
    std::shared_ptr<const ::Rig> getRigHandle() const noexcept { return m_Rig; }

    size_t getMeshCount() const noexcept { return meshes.size(); }
    size_t getMaterialCount() const noexcept { return m_Materials.size(); }
    const Material& getMaterial(size_t index) const { return *m_Materials[index]; }

private:
    // Model data
    std::string directory;
    ImportConfig importConfig;
    ModelFeature modelFeature = ModelFeature::None;
    bool m_Loaded = false;

    std::vector<std::unique_ptr<Mesh>> meshes;
    std::vector<std::unique_ptr<Material>> m_Materials;
    std::unordered_map<std::string, std::unique_ptr<Texture2D>> textures_loaded;
    std::shared_ptr<const ::Rig> m_Rig = std::make_shared<const ::Rig>();

    bool gammaCorrection;

    // State that only exists while a file is being imported (defined in Model.cpp)
    struct LoadContext;

    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(std::string const& path);

    std::vector<Texture2D*> loadMaterialTextures(const aiScene* scene, aiMaterial* mat, aiTextureType type, Texture2D::Texture2DType texType);
    const Material* getOrCreateMaterial(LoadContext& ctx, unsigned int materialIndex);

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode* node, LoadContext& ctx);
    void processMesh(aiMesh* mesh, LoadContext& ctx);

    // helper functions
    static void setVertexBoneDataToDefault(
        std::array<int, MAX_BONE_INFLUENCE>& boneIDs,
        std::array<float, MAX_BONE_INFLUENCE>& weights
    );
    static void setVertexBoneData(
        std::array<int, MAX_BONE_INFLUENCE>& boneIDs,
        std::array<float, MAX_BONE_INFLUENCE>& weights,
        int boneID, float weight
    );
    void extractBoneWeightForVertices(
        LoadContext& ctx,
        std::vector<std::array<int, MAX_BONE_INFLUENCE>>& boneIDs,
        std::vector<std::array<float, MAX_BONE_INFLUENCE>>& weights,
        aiMesh* mesh
    );
    // Force-mode fallbacks (mathematically safe defaults)
    void computeFaceNormals(
        std::vector<glm::vec3>& normals,
        const std::vector<glm::vec3>& positions,
        const std::vector<unsigned int>& triIndices
    );
    void generateArbitraryTangents(
        std::vector<glm::vec3>& tangents,
        std::vector<glm::vec3>& bitangents,
        const std::vector<glm::vec3>& normals,
        size_t vertexCount
    );
};

inline Model::ModelFeature operator|(Model::ModelFeature lhs, Model::ModelFeature rhs)
{
    using T = std::underlying_type_t<Model::ModelFeature>;
    return static_cast<Model::ModelFeature>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline Model::ModelFeature& operator|=(Model::ModelFeature& lhs, Model::ModelFeature rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline Model::ModelFeature operator&(Model::ModelFeature lhs, Model::ModelFeature rhs)
{
    using T = std::underlying_type_t<Model::ModelFeature>;
    return static_cast<Model::ModelFeature>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

inline Model::ModelFeature& operator&=(Model::ModelFeature& lhs, Model::ModelFeature rhs)
{
    lhs = lhs & rhs;
    return lhs;
}