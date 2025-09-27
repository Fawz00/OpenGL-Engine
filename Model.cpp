#include "Model.hpp"

using namespace std;

Model::Model(string const& path, ModelMode mode, bool gamma)
    : mode(mode), gammaCorrection(gamma)
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
        //aiProcess_FlipUVs |
        aiProcess_GenNormals |
		aiProcess_CalcTangentSpace * (!(mode & NO_TANGENTS))
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
		
        if (mode & SKINNED) {
            SetVertexBoneDataToDefault(vertex);
		}

        vertex.Position = AssimpGLMHelpers::GetGLMVec(mesh->mVertices[i]);
        vertex.Normal = mesh->HasNormals()
            ? AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i])
            : glm::vec3(0.0f);

		// Texture coordinates
        if (mesh->mTextureCoords[0] && !(mode & NO_TEXTURES)) {
            vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y);

            if (mesh->mTangents && mesh->mBitangents && !(mode & NO_TANGENTS)) {
                glm::vec3 N = glm::normalize(AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]));
                glm::vec3 T = AssimpGLMHelpers::GetGLMVec(mesh->mTangents[i]);
                glm::vec3 B = AssimpGLMHelpers::GetGLMVec(mesh->mBitangents[i]);

                /*
				// Ensure T is orthogonal to N.
				// Sometimes assimp messes this up.

				// Gram–Schmidt orthogonalize
                T = glm::normalize(T - N * glm::dot(N, T));

                // Calculate handedness (±1)
                float handedness = (glm::dot(glm::cross(N, T), B) < 0.0f) ? -1.0f : 1.0f;

                // Recompute B
                B = glm::cross(N, T) * handedness;
                */

                vertex.Tangent = T;
                vertex.Bitangent = B;
            }
            else {
                vertex.Tangent = glm::vec3(0.0f);
                vertex.Bitangent = glm::vec3(0.0f);
            }
        }
        else {
            vertex.TexCoords = glm::vec2(0.0f);
            vertex.Tangent = glm::vec3(0.0f);
            vertex.Bitangent = glm::vec3(0.0f);
        }

        vertices.push_back(std::move(vertex));
    }

    // --- Indices ---
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        const aiFace& face = mesh->mFaces[i];

        if (mode & FORCE_TRIANGLES && face.mNumIndices != 3) {
            continue;
		}

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
    if (!(mode & NO_TEXTURES)) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        auto diffuseMaps = loadMaterialTextures(scene, material, aiTextureType_DIFFUSE, Texture2D::Texture2DType::TextureDiffuse);
        auto specularMaps = loadMaterialTextures(scene, material, aiTextureType_SPECULAR, Texture2D::Texture2DType::TextureSpecular);

        // Normal map heuristic
        auto normalMaps = loadMaterialTextures(scene, material, aiTextureType_NORMALS, Texture2D::Texture2DType::TextureNormal);
        if (normalMaps.empty()) {
			// fallback: Some models use height maps as normal maps
            normalMaps = loadMaterialTextures(scene, material, aiTextureType_HEIGHT, Texture2D::Texture2DType::TextureNormal);
        }

        auto heightMaps = loadMaterialTextures(scene, material, aiTextureType_DISPLACEMENT, Texture2D::Texture2DType::TextureHeight);

        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
    }

	// --- Bones ---
    if (mode & SKINNED) {
        ExtractBoneWeightForVertices(vertices, mesh, scene);
	}

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
    Texture2D::Texture2DType texType
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

        // --- Embedded? ---
        const aiTexture* aiTex = scene->GetEmbeddedTexture(texID.c_str());
        if (aiTex) {
            std::unique_ptr<Texture2D> texture;
            if (aiTex->mHeight == 0) {
                // Compressed (JPG/PNG)
                unsigned char* data = reinterpret_cast<unsigned char*>(aiTex->pcData);
                size_t size = aiTex->mWidth;
                texture = std::make_unique<Texture2D>(data, size, texType);
            }
            else {
                // Uncompressed (raw BGRA/RGBA)
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
            // --- External ---
            std::string fullPath = texID;
            if (fullPath.find(':') == std::string::npos && fullPath[0] != '/') {
                fullPath = directory + "/" + texID;
            }
            auto texture = std::make_unique<Texture2D>(fullPath, true, texType);
            textures.push_back(texture.get());
            textures_loaded.emplace(texID, std::move(texture));
        }
    }

    return textures;
}

void Model::SetVertexBoneDataToDefault(Vertex& vertex)
{
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
        vertex.m_BoneIDs[i] = -1;
        vertex.m_Weights[i] = 0.0f;
    }
}

void Model::SetVertexBoneData(Vertex& vertex, int boneID, float weight)
{
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
    {
        if (vertex.m_BoneIDs[i] < 0)
        {
            vertex.m_Weights[i] = weight;
            vertex.m_BoneIDs[i] = boneID;
            break;
        }
    }
}

void Model::ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene)
{
    auto& boneInfoMap = m_BoneInfoMap;
    int& boneCount = m_BoneCounter;

    for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
    {
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
        if (boneInfoMap.find(boneName) == boneInfoMap.end())
        {
            BoneInfo newBoneInfo;
            newBoneInfo.id = boneCount;
            newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
            boneInfoMap[boneName] = newBoneInfo;
            boneID = boneCount;
            boneCount++;
        }
        else
        {
            boneID = boneInfoMap[boneName].id;
        }
        assert(boneID != -1);
        auto weights = mesh->mBones[boneIndex]->mWeights;
        int numWeights = mesh->mBones[boneIndex]->mNumWeights;

        for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
        {
            int vertexId = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;
            assert(vertexId <= vertices.size());
            SetVertexBoneData(vertices[vertexId], boneID, weight);
        }
    }
}