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
#include "Debug.hpp"

using namespace std;

class Model
{
public:
    // constructor, expects a filepath to a 3D model.
    explicit Model(string const& path, bool gamma = false);
    ~Model() = default;

    // No copy
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    // Move allowed
    Model(Model&&) noexcept = default;
    Model& operator=(Model&&) noexcept = default;

    // draws the model, and thus all its meshes
    void Draw(Shader& shader);

private:
    // Model data
    std::vector<std::unique_ptr<Mesh>> meshes;
    std::unordered_map<std::string, std::unique_ptr<Texture2D>> textures_loaded;
    std::string directory;
    bool gammaCorrection;

    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const& path);

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode* node, const aiScene* scene);
    void processMesh(aiMesh* mesh, const aiScene* scene);
    vector<Texture2D*> loadMaterialTextures(const aiScene* scene, aiMaterial* mat, aiTextureType type, Texture2D::TextureType texType);
};