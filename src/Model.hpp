#pragma once

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <unordered_map>

#include "stb_image.h"
#include "Mesh.hpp"
#include "Shader.hpp"
#include "Texture2D.hpp"
#include "AssimpGLMHelpers.hpp"
#include "Debug.hpp"

struct BoneInfo
{
    /*id is index in finalBoneMatrices*/
    int id;
    /*offset matrix transforms vertex from model space to bone space*/
    glm::mat4 offset;
};

class Model
{
public:
    enum ConfigOption {
        Never,
        IfAvailable,
        Force
    };

    class ImportConfig {
        public:
            ImportConfig() = default;

			// Setters for each configuration option
			ImportConfig& setPositions(ConfigOption option) { positions = option; return *this; }
            ImportConfig& setTextures(ConfigOption option) { textures = option; return *this; }
            ImportConfig& setNormals(ConfigOption option) { normals = option; return *this; }
            ImportConfig& setTangents(ConfigOption option) { tangents = option; return *this; }
            ImportConfig& setSkinning(ConfigOption option) { skinning = option; return *this; }
            ImportConfig& setTriangulation(ConfigOption option) { triangulation = option; return *this; }
            ImportConfig& setLines(ConfigOption option) { lines = option; return *this; }
            ImportConfig& setPoints(ConfigOption option) { points = option; return *this; }

			ImportConfig& setGammaCorrection(bool enable) { gammaCorrection = enable; return *this; }

			// Getters for each configuration option
			ConfigOption getPositions() const { return positions; }
            ConfigOption getTextures() const { return textures; }
            ConfigOption getNormals() const { return normals; }
            ConfigOption getTangents() const { return tangents; }
            ConfigOption getSkinning() const { return skinning; }
            ConfigOption getTriangulation() const { return triangulation; }
            ConfigOption getLines() const { return lines; }
			ConfigOption getPoints() const { return points; }

			bool isGammaCorrectionEnabled() const { return gammaCorrection; }
        private:
            ConfigOption positions = IfAvailable;
            ConfigOption textures = IfAvailable;
            ConfigOption normals = IfAvailable;
            ConfigOption tangents = IfAvailable;
            ConfigOption skinning = IfAvailable;
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
        Skinning = 1 << 4,
        Animation = 1 << 5,
    };

    // constructor, expects a filepath to a 3D model.
    explicit Model(std::string const& path, ImportConfig config = ImportConfig());
    ~Model() = default;

    // No copy
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    // Move allowed
    Model(Model&&) noexcept = default;
    Model& operator=(Model&&) noexcept = default;

    void draw(Shader& shader);

    // Getters
    auto& getBoneInfoMap() { return m_BoneInfoMap; }
    int& getBoneCount() { return m_BoneCounter; }
    const std::string& getPath() const { return directory; }
    ModelFeature getModelFeature() const { return modelFeature; }

    const std::unordered_map<std::string, std::string>& getNodeParentMap() const { return m_NodeParentMap; }
    const std::string& getRootNodeName() const { return m_RootNodeName; }

private:
    // Model data
    std::string directory;
    ImportConfig importConfig;
	ModelFeature modelFeature = ModelFeature::None;

    std::vector<std::unique_ptr<Mesh>> meshes;
    std::unordered_map<std::string, std::unique_ptr<Texture2D>> textures_loaded;
    std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;
    std::unordered_map<std::string, std::string> m_NodeParentMap;
    std::string m_RootNodeName;
    int m_BoneCounter = 0;

    bool gammaCorrection;

    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(std::string const& path);

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    std::vector<Texture2D*> loadMaterialTextures(const aiScene* scene, aiMaterial* mat, aiTextureType type, Texture2D::Texture2DType texType);
    void processNode(aiNode* node, const aiScene* scene);
    void processMesh(aiMesh* mesh, const aiScene* scene);
    void buildNodeParentMap(const aiNode* node, const std::string& parentName);

    // helper functions
    void setVertexBoneDataToDefault(
        std::array<int, MAX_BONE_INFLUENCE>& boneIDs,
        std::array<float, MAX_BONE_INFLUENCE>& weights
    );
    void setVertexBoneData(
        std::array<int, MAX_BONE_INFLUENCE>& boneIDs,
        std::array<float, MAX_BONE_INFLUENCE>& weights,
        int boneID, float weight);
    void extractBoneWeightForVertices(
        std::vector<std::array<int, MAX_BONE_INFLUENCE>>& boneIDs,
        std::vector<std::array<float, MAX_BONE_INFLUENCE>>& weights,
        aiMesh* mesh, const aiScene* scene);
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