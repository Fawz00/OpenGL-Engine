#include "Model.hpp"

#include <cmath>
#include <array>

using namespace std;

Model::Model(string const& path, ImportConfig config)
    : importConfig(config)
{
    loadModel(path);
}

void Model::draw(Shader& shader)
{
    for (auto& mesh : meshes) {
        mesh->draw(shader);
    }
}

void Model::loadModel(string const& path)
{
    // Read file via ASSIMP
    Assimp::Importer importer;

    const bool wantNormals = !(importConfig.getNormals() == ConfigOption::Never);
    const bool wantTangents = !(importConfig.getTangents() == ConfigOption::Never);

    unsigned int postProcessFlags = aiProcess_Triangulate;
    if (wantNormals) {
        postProcessFlags |= aiProcess_GenSmoothNormals | aiProcess_GenNormals;
    }
    if (wantTangents) {
        postProcessFlags |= aiProcess_CalcTangentSpace;
    }

    const aiScene* scene = importer.ReadFile(path, postProcessFlags);

    // Check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        Debug::logError("ERROR::ASSIMP:: " + std::string(importer.GetErrorString()) + "\n");
        return;
    }

    // Retrieve the directory path of the filepath
    directory = path.substr(0, path.find_last_of('/'));

    m_RootNodeName = scene->mRootNode->mName.C_Str();
    buildNodeParentMap(scene->mRootNode, "");

    // Process ASSIMP's root node recursively
    try {
        processNode(scene->mRootNode, scene);
    }
    catch (const std::exception& e) {
        Debug::logError("ERROR::MODEL:: " + std::string(e.what()) + "\n");
        return;
    }
}

void Model::buildNodeParentMap(const aiNode* node, const string& parentName)
{
    if (!node) return;

    string name = node->mName.C_Str();
    m_NodeParentMap[name] = parentName;

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        buildNodeParentMap(node->mChildren[i], name);
    }
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
    vector<glm::vec3> positions;
    vector<glm::vec3> normals;
    vector<glm::vec2> texCoords;
    vector<glm::vec3> tangents;
    vector<glm::vec3> bitangents;

    vector<array<int, MAX_BONE_INFLUENCE>> boneIDs;
    vector<array<float, MAX_BONE_INFLUENCE>> weights;

    vector<unsigned int> triIndices;
    vector<unsigned int> lineIndices;
    vector<unsigned int> pointIndices;
    vector<Texture2D*> textures;

    // --- Feature resolution ---
    bool hasPositions = (mesh->HasPositions() || importConfig.getPositions() == Model::ConfigOption::Force)
        && !(importConfig.getPositions() == Model::ConfigOption::Never);

    bool hasNormals = (mesh->HasNormals() || importConfig.getNormals() == Model::ConfigOption::Force)
        && hasPositions
        && !(importConfig.getNormals() == Model::ConfigOption::Never);

    bool hasTexCoords = mesh->HasTextureCoords(0) && hasPositions
        && !(importConfig.getTextures() == Model::ConfigOption::Never);

    bool hasNativeTangents = mesh->HasTangentsAndBitangents() && hasTexCoords;
    bool hasTangents = (hasNativeTangents || importConfig.getTangents() == Model::ConfigOption::Force)
        && hasPositions
        && !(importConfig.getTangents() == Model::ConfigOption::Never);

    bool hasRig = mesh->HasBones() || (importConfig.getRig() == Model::ConfigOption::Force);

    if (hasPositions) modelFeature |= ModelFeature::Positions;
    if (hasTexCoords) modelFeature |= ModelFeature::Textures;
    if (hasNormals)   modelFeature |= ModelFeature::Normals;
    if (hasTangents)  modelFeature |= ModelFeature::Tangents;
    if (hasRig)  modelFeature |= ModelFeature::Rig;

    positions.reserve(mesh->mNumVertices);
    if (hasNormals)   normals.reserve(mesh->mNumVertices);
    if (hasTexCoords) texCoords.reserve(mesh->mNumVertices);
    if (hasTangents) {
        tangents.reserve(mesh->mNumVertices);
        bitangents.reserve(mesh->mNumVertices);
    }
    if (hasRig) {
        boneIDs.resize(mesh->mNumVertices);
        weights.resize(mesh->mNumVertices);
    }

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        if (hasRig) {
            setVertexBoneDataToDefault(boneIDs[i], weights[i]);
        }

        if (mesh->HasPositions()) {
            positions.push_back(AssimpGLMHelpers::GetGLMVec(mesh->mVertices[i]));
        }
        else if (hasPositions) {
            positions.push_back(glm::vec3(0.0f));
        }

        if (mesh->HasNormals()) {
            normals.push_back(AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]));
        }

        // Texture coordinates
        if (mesh->mTextureCoords[0] && hasTexCoords) {
            texCoords.push_back(glm::vec2(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            ));

            if (mesh->mTangents && mesh->mBitangents && hasNativeTangents) {
                glm::vec3 N = glm::normalize(AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]));
                glm::vec3 T = AssimpGLMHelpers::GetGLMVec(mesh->mTangents[i]);
                glm::vec3 B = AssimpGLMHelpers::GetGLMVec(mesh->mBitangents[i]);

                T = glm::normalize(T - N * glm::dot(N, T));

                float handedness = (glm::dot(glm::cross(N, T), B) < 0.0f) ? -1.0f : 1.0f;
                B = glm::cross(N, T) * handedness;

                tangents.push_back(T);
                bitangents.push_back(B);
            }
        }
    }

    // --- Indices ---
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        const aiFace& face = mesh->mFaces[i];

        if (importConfig.getTriangulation() == Model::ConfigOption::Force && face.mNumIndices < 3) {
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
        else {
            if (importConfig.getTriangulation() == Model::ConfigOption::Force) {
                for (unsigned int j = 1; j < face.mNumIndices - 1; j++) {
                    triIndices.push_back(face.mIndices[0]);
                    triIndices.push_back(face.mIndices[j]);
                    triIndices.push_back(face.mIndices[j + 1]);
                }
            }
            else {
                continue;
            }
        }
    }

	// --- Compute normals if not present ---
    if (hasNormals && !mesh->HasNormals()) {
        normals.assign(mesh->mNumVertices, glm::vec3(0.0f));
        computeFaceNormals(normals, positions, triIndices);
    }

	// --- Generate tangents and bitangents if not present ---
    if (hasTangents && !hasNativeTangents) {
		Debug::logWarn("Model::processMesh: Tangents and bitangents are not present in the mesh. Generating arbitrary tangents and bitangents.\n");
        tangents.assign(mesh->mNumVertices, glm::vec3(0.0f));
        bitangents.assign(mesh->mNumVertices, glm::vec3(0.0f));
        generateArbitraryTangents(tangents, bitangents, normals, mesh->mNumVertices);
    }

    // --- Textures ---
    if (hasTexCoords) {
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
    if (hasRig) {
        extractBoneWeightForVertices(boneIDs, weights, mesh, scene);

		// If rigging is forced and no bones are present, assign all vertices to a single root bone
        if (importConfig.getRig() == Model::ConfigOption::Force) {
            int rootBoneID = -1;
            for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
                if (boneIDs[v][0] == -1) {
                    if (rootBoneID == -1) {
                        rootBoneID = ensureRootBoneInfo();
                    }
                    boneIDs[v][0] = rootBoneID;
                    weights[v][0] = 1.0f;
                }
            }
        }
    }

    // --- Submeshes ---
    vector<SubMesh> subMeshes;
    if (!triIndices.empty())   subMeshes.push_back({ move(triIndices), GL_TRIANGLES });
    if (!lineIndices.empty())  subMeshes.push_back({ move(lineIndices), GL_LINES });
    if (!pointIndices.empty()) subMeshes.push_back({ move(pointIndices), GL_POINTS });

    // --- Create single Mesh with multiple submeshes ---
    Mesh meshObj(
        mesh->mNumVertices,
        move(subMeshes),
        move(textures)
    );
    meshObj.setPosition(move(positions));
    if (hasNormals)   meshObj.setNormals(move(normals));
    if (hasTexCoords) meshObj.setTexCoords(0, move(texCoords));
    if (hasTangents) {
        meshObj.setTangents(move(tangents));
        meshObj.setBitangents(move(bitangents));
    }
    if (hasRig) {
        meshObj.setBoneIDs(move(boneIDs));
        meshObj.setWeights(move(weights));
    }

    meshes.push_back(make_unique<Mesh>(move(meshObj)));
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

        string texID = str.C_Str(); // "*0" for embedded, "texture.jpg" for file

        // Check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
        auto it = textures_loaded.find(texID);
        if (it != textures_loaded.end()) {
            textures.push_back(it->second.get());
            continue;
        }

        // --- Embedded? ---
        const aiTexture* aiTex = scene->GetEmbeddedTexture(texID.c_str());
        if (aiTex) {
            unique_ptr<Texture2D> texture;
            if (aiTex->mHeight == 0) {
                // Compressed (JPG/PNG)
                unsigned char* data = reinterpret_cast<unsigned char*>(aiTex->pcData);
                size_t size = aiTex->mWidth;
                texture = make_unique<Texture2D>(data, size, texType);
            }
            else {
                // Uncompressed (raw BGRA/RGBA)
                unsigned char* data = reinterpret_cast<unsigned char*>(aiTex->pcData);
                int width = aiTex->mWidth;
                int height = aiTex->mHeight;
                int channels = 4;
                texture = make_unique<Texture2D>(data, width, height, channels, texType);
            }

            textures.push_back(texture.get());
            textures_loaded.emplace(texID, move(texture));
        }
        else {
            // --- External ---
            string fullPath = texID;
            if (!fullPath.empty() &&
                fullPath.find(':') == string::npos &&
                fullPath[0] != '/'
            ) {
                fullPath = directory + "/" + texID;
            }
            auto texture = make_unique<Texture2D>(fullPath, true, texType);
            textures.push_back(texture.get());
            textures_loaded.emplace(texID, move(texture));
        }
    }

    return textures;
}

void Model::setVertexBoneDataToDefault(
    std::array<int, MAX_BONE_INFLUENCE>& boneIDs,
    std::array<float, MAX_BONE_INFLUENCE>& weights
)
{
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
        boneIDs[i] = -1;
        weights[i] = 0.0f;
    }
}

void Model::setVertexBoneData(
    std::array<int, MAX_BONE_INFLUENCE>& boneIDs,
    std::array<float, MAX_BONE_INFLUENCE>& weights,
    int boneID, float weight)
{
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
    {
        if (boneIDs[i] < 0)
        {
            boneIDs[i] = boneID;
            weights[i] = weight;
            break;
        }
    }
}

void Model::extractBoneWeightForVertices(
    std::vector<std::array<int, MAX_BONE_INFLUENCE>>& boneIDs,
    std::vector<std::array<float, MAX_BONE_INFLUENCE>>& weights,
    aiMesh* mesh, const aiScene* scene)
{
    auto& boneInfoMap = m_BoneInfoMap;
    int& boneCount = m_BoneCounter;

    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
    {
        const aiBone* aiBone = mesh->mBones[boneIndex];
        std::string boneName = aiBone->mName.C_Str();
        int boneID = -1;

        auto [it, inserted] = boneInfoMap.try_emplace(
            boneName,
            BoneInfo{ boneCount, AssimpGLMHelpers::ConvertMatrixToGLMFormat(aiBone->mOffsetMatrix) }
        );

        if (inserted)
            boneID = boneCount++;
        else
            boneID = it->second.id;

        if (aiBone->mNumWeights == 0)
            continue;

        for (unsigned int weightIndex = 0; weightIndex < aiBone->mNumWeights; ++weightIndex)
        {
            unsigned int vertexId = aiBone->mWeights[weightIndex].mVertexId;
            float weight = aiBone->mWeights[weightIndex].mWeight;

            if (vertexId >= boneIDs.size()) {
                continue;
            }

            setVertexBoneData(boneIDs[vertexId], weights[vertexId], boneID, weight);
        }
    }
}

void Model::computeFaceNormals(
    std::vector<glm::vec3>& normals,
    const std::vector<glm::vec3>& positions,
    const std::vector<unsigned int>& triIndices)
{
    for (size_t f = 0; f + 2 < triIndices.size(); f += 3) {
        unsigned int i0 = triIndices[f];
        unsigned int i1 = triIndices[f + 1];
        unsigned int i2 = triIndices[f + 2];

        const glm::vec3& p0 = positions[i0];
        const glm::vec3& p1 = positions[i1];
        const glm::vec3& p2 = positions[i2];

        glm::vec3 faceNormal = glm::cross(p1 - p0, p2 - p0);

        normals[i0] += faceNormal;
        normals[i1] += faceNormal;
        normals[i2] += faceNormal;
    }

    for (auto& n : normals) {
        float lenSq = glm::dot(n, n);
        n = (lenSq > 1e-12f) ? (n / std::sqrt(lenSq)) : glm::vec3(0.0f, 1.0f, 0.0f);
    }
}

void Model::generateArbitraryTangents(
    std::vector<glm::vec3>& tangents,
    std::vector<glm::vec3>& bitangents,
    const std::vector<glm::vec3>& normals,
    size_t vertexCount)
{
    static const glm::vec3 kWorldUp(0.0f, 1.0f, 0.0f);
    static const glm::vec3 kWorldRight(1.0f, 0.0f, 0.0f);

    for (size_t i = 0; i < vertexCount; i++) {
        glm::vec3 n = (i < normals.size()) ? normals[i] : glm::vec3(0.0f, 1.0f, 0.0f);
        if (glm::dot(n, n) < 1e-12f) {
            n = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        glm::vec3 reference = (std::abs(glm::dot(n, kWorldUp)) > 0.99f) ? kWorldRight : kWorldUp;

        glm::vec3 t = glm::normalize(glm::cross(reference, n));
        glm::vec3 b = glm::cross(n, t);

        tangents[i] = t;
        bitangents[i] = b;
    }
}

int Model::ensureRootBoneInfo()
{
    auto it = m_BoneInfoMap.find(m_RootNodeName);
    if (it != m_BoneInfoMap.end()) {
        return it->second.id;
    }

    int id = m_BoneCounter++;
    m_BoneInfoMap[m_RootNodeName] = BoneInfo{ id, glm::mat4(1.0f) };

	Debug::logWarn("Model::ensureRootBoneInfo: No bones found in the model. Created a root bone with name '" + m_RootNodeName + "' and ID " + std::to_string(id) + ".\n");

    return id;
}