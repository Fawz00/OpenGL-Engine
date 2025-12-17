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
    enum ModelMode : uint32_t {
		NONE            = 0,
        SKINNED         = 1 << 0,
		NO_TEXTURES     = 1 << 1,
		NO_TANGENTS     = 1 << 2,
		FORCE_TRIANGLES = 1 << 3,
    };

    // constructor, expects a filepath to a 3D model.
    explicit Model(std::string const& path, ModelMode mode = NONE, bool gamma = false);
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
	uint32_t getMode() const { return mode; }

private:
    // Model data
    std::string directory;
	ModelMode mode;

    std::vector<std::unique_ptr<Mesh>> meshes;
    std::unordered_map<std::string, std::unique_ptr<Texture2D>> textures_loaded;
    std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;

    bool gammaCorrection;

    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(std::string const& path);

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode* node, const aiScene* scene);
    void processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture2D*> loadMaterialTextures(const aiScene* scene, aiMaterial* mat, aiTextureType type, Texture2D::Texture2DType texType);

	// helper functions
    void setVertexBoneDataToDefault(Vertex& vertex);
    void setVertexBoneData(Vertex& vertex, int boneID, float weight);
    void extractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene);
};