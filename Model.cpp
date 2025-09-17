#include "Model.hpp"

Model::Model(string const& path, bool gamma)
    : gammaCorrection(gamma)
{
    loadModel(path);
}

void Model::Draw(Shader& shader)
{
    for (auto& mesh : meshes) {
        mesh->Draw(shader);
    }
}

void Model::loadModel(string const& path)
{
    // Read file via ASSIMP
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace
    );

    // Check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
        return;
    }

    // Retrieve the directory path of the filepath
    directory = path.substr(0, path.find_last_of('/'));

    // Process ASSIMP's root node recursively
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    Debug::log("Processing node: " + string(node->mName.C_Str()));

    // Process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene);
    }

    // After we've processed all of the meshes, we then recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

void Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> triIndices;
    std::vector<unsigned int> lineIndices;
    std::vector<unsigned int> pointIndices;
    std::vector<Texture2D*> textures;

    // --- Vertex data ---
    vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.Position = glm::vec3(mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z);
        vertex.Normal = mesh->HasNormals() ?
            glm::vec3(mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z) : glm::vec3(0.0f);

        if (mesh->mTextureCoords[0]) {
            vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y);
            if (mesh->mTangents)
                vertex.Tangent = glm::vec3(mesh->mTangents[i].x,
                    mesh->mTangents[i].y,
                    mesh->mTangents[i].z);
            if (mesh->mBitangents)
                vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x,
                    mesh->mBitangents[i].y,
                    mesh->mBitangents[i].z);
        }
        else {
            vertex.TexCoords = glm::vec2(0.0f);
        }
        vertices.push_back(std::move(vertex));
    }

    // --- Indices ---
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        const aiFace& face = mesh->mFaces[i];
        if (face.mNumIndices == 1) {
            pointIndices.push_back(face.mIndices[0]);
        }
        else if (face.mNumIndices == 2) {
            lineIndices.push_back(face.mIndices[0]);
            lineIndices.push_back(face.mIndices[1]);
        }
        else if (face.mNumIndices == 3) {
            triIndices.push_back(face.mIndices[0]);
            triIndices.push_back(face.mIndices[1]);
            triIndices.push_back(face.mIndices[2]);
        }
		// If the face has more than 3 indices, we ignore it for now
    }

    // --- Textures ---
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    auto diffuseMaps = loadMaterialTextures(scene, material, aiTextureType_DIFFUSE, Texture2D::TextureType::TextureDiffuse);
    auto specularMaps = loadMaterialTextures(scene, material, aiTextureType_SPECULAR, Texture2D::TextureType::TextureSpecular);
    auto normalMaps = loadMaterialTextures(scene, material, aiTextureType_HEIGHT, Texture2D::TextureType::TextureNormal);
    auto heightMaps = loadMaterialTextures(scene, material, aiTextureType_AMBIENT, Texture2D::TextureType::TextureHeight);

    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    // --- Submeshes ---
    std::vector<SubMesh> subMeshes;
    if (!triIndices.empty()) {
        subMeshes.push_back({ std::move(triIndices), GL_TRIANGLES });
    }
    if (!lineIndices.empty()) {
        subMeshes.push_back({ std::move(lineIndices), GL_LINES });
    }
    if (!pointIndices.empty()) {
        subMeshes.push_back({ std::move(pointIndices), GL_POINTS });
    }

    // --- Create single Mesh with multiple submeshes ---
    meshes.push_back(std::make_unique<Mesh>(
        std::move(vertices),
        std::move(subMeshes),
        std::move(textures)
    ));
}

vector<Texture2D*> Model::loadMaterialTextures(
    const aiScene* scene,
    aiMaterial* mat,
    aiTextureType type,
    Texture2D::TextureType texType
) {
    vector<Texture2D*> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);

		std::string texID = str.C_Str(); // "*0" for embedded, "texture.jpg" for file

		// Check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
        auto it = textures_loaded.find(texID);
        if (it != textures_loaded.end()) {
            textures.push_back(it->second.get());
            continue;
        }

		// Check if texture is embedded or from file
        if (texID[0] == '*') {
            int texIndex = atoi(texID.c_str() + 1);
            aiTexture* aiTex = scene->mTextures[texIndex];

            std::unique_ptr<Texture2D> texture;
            if (aiTex->mHeight == 0) {
				// Compressed texture
                unsigned char* data = reinterpret_cast<unsigned char*>(aiTex->pcData);
                size_t size = aiTex->mWidth;
                texture = std::make_unique<Texture2D>(data, size, texType);
            }
            else {
				// Uncompressed texture
                unsigned char* data = reinterpret_cast<unsigned char*>(aiTex->pcData);
                int width = aiTex->mWidth;
                int height = aiTex->mHeight;
                int channels = 4;
                texture = std::make_unique<Texture2D>(data, width, height, channels, texType);
            }

            textures.push_back(texture.get());
            textures_loaded.emplace(texID, std::move(texture));
        }
        else {
            std::string fullPath = directory + "/" + texID;
            auto texture = std::make_unique<Texture2D>(fullPath, true, texType);
            textures.push_back(texture.get());
            textures_loaded.emplace(texID, std::move(texture));
        }
    }

    return textures;
}
