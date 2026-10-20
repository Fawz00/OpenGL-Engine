#include "Model.hpp"

#include <assimp/config.h>

#include <cmath>
#include <cstring>
#include <array>

using namespace std;

// State that only lives while a file is imported.
struct Model::LoadContext {
    const aiScene* scene = nullptr;
    ::Rig::Builder rig;                 // collects bones; becomes the immutable Rig at the end
    vector<int32_t> materialLookup;     // scene material index -> index in m_Materials (-1 = not created)
    string rootNodeName;
};

namespace {
    // aiVector3D is 3 floats when Assimp is built with float precision (default).
    // In that case a single memcpy replaces a per-vertex loop.
    void copyVec3(const aiVector3D* src, glm::vec3* dst, size_t count)
    {
        if constexpr (sizeof(aiVector3D) == sizeof(glm::vec3) && is_same_v<ai_real, float>) {
            std::memcpy(dst, src, count * sizeof(glm::vec3));
        }
        else {
            for (size_t i = 0; i < count; ++i) dst[i] = AssimpGLMHelpers::GetGLMVec(src[i]);
        }
    }
}

Model::Model(string const& path)
    : Model(path, ImportConfig())
{
}

Model::Model(string const& path, ImportConfig config)
    : importConfig(config)
{
    loadModel(path);
}

void Model::draw(Shader& shader) const
{
    const Material* current = nullptr;
    bool first = true;

    for (const auto& mesh : meshes) {
        // Only touch texture state when the material changes between consecutive meshes.
        const Material* material = mesh->getMaterial();
        if (first || material != current) {
            if (material) material->bind(shader);
            else Material::bindNone(shader);
            current = material;
            first = false;
        }
        mesh->drawGeometry();
    }

    if (!first) {
        Material::unbindAll();
    }
}

void Model::loadModel(string const& path)
{
    // Read file via ASSIMP
    Assimp::Importer importer;

    const bool wantNormals = !(importConfig.getNormals() == ConfigOption::Never);
    const bool wantTangents = !(importConfig.getTangents() == ConfigOption::Never);
    const bool wantRig = !(importConfig.getRig() == ConfigOption::Never);

    unsigned int postProcessFlags = aiProcess_Triangulate;
    if (wantNormals) {
        // Exactly one of the two: Assimp rejects aiProcess_GenNormals | aiProcess_GenSmoothNormals.
        // Flat is the default (it is also what the old flag combination effectively produced).
        postProcessFlags |= (importConfig.getNormalMode() == NormalMode::Smooth)
            ? aiProcess_GenSmoothNormals
            : aiProcess_GenNormals;
    }
    if (wantTangents) {
        postProcessFlags |= aiProcess_CalcTangentSpace;
    }
    if (wantRig) {
        // At most MAX_BONE_INFLUENCE influences per vertex, with weights renormalised by Assimp.
        // Without this, extra weights were dropped and the remaining ones no longer summed to 1.
        postProcessFlags |= aiProcess_LimitBoneWeights;
        importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, MAX_BONE_INFLUENCE);
    }

    const aiScene* scene = importer.ReadFile(path, postProcessFlags);

    // Check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        Debug::logError("ERROR::ASSIMP:: " + std::string(importer.GetErrorString()) + "\n");
        return;
    }

    // Retrieve the directory path of the filepath
    directory = path.substr(0, path.find_last_of('/'));

    LoadContext ctx;
    ctx.scene = scene;
    ctx.materialLookup.assign(scene->mNumMaterials, -1);
    ctx.rootNodeName = scene->mRootNode->mName.C_Str();
    m_Materials.reserve(scene->mNumMaterials);

    // Process ASSIMP's root node recursively
    try {
        processNode(scene->mRootNode, ctx);
    }
    catch (const std::exception& e) {
        Debug::logError("ERROR::MODEL:: " + std::string(e.what()) + "\n");
        return;
    }

    // Bones are now all registered: build the immutable rig (empty when there are no bones).
    m_Rig = std::make_shared<const ::Rig>(ctx.rig.build(scene->mRootNode));
    m_Loaded = true;
}

void Model::processNode(aiNode* node, LoadContext& ctx)
{
    // Process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = ctx.scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, ctx);
    }

    // After we've processed all of the meshes, we then recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], ctx);
    }
}

void Model::processMesh(aiMesh* mesh, LoadContext& ctx) {
    const unsigned int vertexCount = mesh->mNumVertices;

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

    // --- Feature resolution ---
    const bool meshHasPositions = mesh->HasPositions();
    const bool meshHasNormals = mesh->HasNormals();

    bool hasPositions = (meshHasPositions || importConfig.getPositions() == Model::ConfigOption::Force)
        && !(importConfig.getPositions() == Model::ConfigOption::Never);

    bool hasNormals = (meshHasNormals || importConfig.getNormals() == Model::ConfigOption::Force)
        && hasPositions
        && !(importConfig.getNormals() == Model::ConfigOption::Never);

    bool hasTexCoords = mesh->HasTextureCoords(0) && hasPositions
        && !(importConfig.getTextures() == Model::ConfigOption::Never);

    bool hasNativeTangents = mesh->HasTangentsAndBitangents() && hasTexCoords;
    bool hasTangents = (hasNativeTangents || importConfig.getTangents() == Model::ConfigOption::Force)
        && hasPositions
        && !(importConfig.getTangents() == Model::ConfigOption::Never);

    // ConfigOption::Never now also disables the rig (it was ignored before when the mesh had bones).
    bool hasRig = (mesh->HasBones() || importConfig.getRig() == Model::ConfigOption::Force)
        && !(importConfig.getRig() == Model::ConfigOption::Never);

    if (hasPositions) modelFeature |= ModelFeature::Positions;
    if (hasTexCoords) modelFeature |= ModelFeature::Textures;
    if (hasNormals)   modelFeature |= ModelFeature::Normals;
    if (hasTangents)  modelFeature |= ModelFeature::Tangents;
    if (hasRig)       modelFeature |= ModelFeature::Rig;

    // --- Vertex data (conditions hoisted out of the per-vertex loops) ---
    if (meshHasPositions) {
        positions.resize(vertexCount);
        copyVec3(mesh->mVertices, positions.data(), vertexCount);
    }
    else if (hasPositions) {
        positions.assign(vertexCount, glm::vec3(0.0f));
    }

    if (hasNormals && meshHasNormals) {
        normals.resize(vertexCount);
        copyVec3(mesh->mNormals, normals.data(), vertexCount);
    }

    if (hasTexCoords) {
        texCoords.resize(vertexCount);
        const aiVector3D* uv = mesh->mTextureCoords[0];
        for (unsigned int i = 0; i < vertexCount; i++) {
            texCoords[i] = glm::vec2(uv[i].x, uv[i].y);
        }

        if (mesh->mTangents && mesh->mBitangents && hasNativeTangents) {
            tangents.resize(vertexCount);
            bitangents.resize(vertexCount);

            for (unsigned int i = 0; i < vertexCount; i++) {
                glm::vec3 N = glm::normalize(AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]));
                glm::vec3 T = AssimpGLMHelpers::GetGLMVec(mesh->mTangents[i]);
                glm::vec3 B = AssimpGLMHelpers::GetGLMVec(mesh->mBitangents[i]);

                T = glm::normalize(T - N * glm::dot(N, T));

                float handedness = (glm::dot(glm::cross(N, T), B) < 0.0f) ? -1.0f : 1.0f;
                B = glm::cross(N, T) * handedness;

                tangents[i] = T;
                bitangents[i] = B;
            }
        }
    }

    // --- Indices ---
    triIndices.reserve(static_cast<size_t>(mesh->mNumFaces) * 3);
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
    if (hasNormals && !meshHasNormals) {
        normals.assign(vertexCount, glm::vec3(0.0f));
        computeFaceNormals(normals, positions, triIndices);
    }

    // --- Generate tangents and bitangents if not present ---
    if (hasTangents && !hasNativeTangents) {
        Debug::logWarn("Model::processMesh: Tangents and bitangents are not present in the mesh. Generating arbitrary tangents and bitangents.\n");
        tangents.assign(vertexCount, glm::vec3(0.0f));
        bitangents.assign(vertexCount, glm::vec3(0.0f));
        generateArbitraryTangents(tangents, bitangents, normals, vertexCount);
    }

    // --- Material (textures) ---
    const Material* material = nullptr;
    if (hasTexCoords) {
        material = getOrCreateMaterial(ctx, mesh->mMaterialIndex);
    }

    // --- Bones ---
    if (hasRig) {
        boneIDs.resize(vertexCount);
        weights.resize(vertexCount);
        for (unsigned int i = 0; i < vertexCount; i++) {
            setVertexBoneDataToDefault(boneIDs[i], weights[i]);
        }

        extractBoneWeightForVertices(ctx, boneIDs, weights, mesh);

        // If rigging is forced and vertices have no bone, assign them to a single root bone
        if (importConfig.getRig() == Model::ConfigOption::Force) {
            int rootBoneID = -1;
            for (unsigned int v = 0; v < vertexCount; v++) {
                if (boneIDs[v][0] == -1) {
                    if (rootBoneID == -1) {
                        rootBoneID = ctx.rig.ensureRootBone(ctx.rootNodeName);
                    }
                    boneIDs[v][0] = rootBoneID;
                    weights[v][0] = 1.0f;
                }
            }
        }
    }

    // --- Weight sanity ---
    // The skinning shader computes sum(w_i * (M_i * p)). If the weights of a vertex sum to less than 1,
    // the result is scaled toward the world origin, which looks like vertices "stretching to (0,0,0)".
    // Weights that were dropped (more influences than MAX_BONE_INFLUENCE, bad vertex ids) are the usual cause.
    if (hasRig) {
        unsigned int renormalized = 0;
        for (unsigned int v = 0; v < vertexCount; v++) {
            float sum = 0.0f;
            for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
                if (boneIDs[v][i] >= 0) sum += weights[v][i];
            }
            if (sum > 0.0f && std::abs(sum - 1.0f) > 1e-3f) {
                const float inv = 1.0f / sum;
                for (int i = 0; i < MAX_BONE_INFLUENCE; i++) weights[v][i] *= inv;
                renormalized++;
            }
        }
        if (renormalized > 0) {
            Debug::logWarn("Model::processMesh: " + std::to_string(renormalized) + " of " + std::to_string(vertexCount)
                + " vertices had bone weights that did not sum to 1. They were renormalized.\n");
        }
    }

    // --- Submeshes ---
    vector<SubMesh> subMeshes;
    if (!triIndices.empty())   subMeshes.push_back({ move(triIndices), GL_TRIANGLES });
    if (!lineIndices.empty())  subMeshes.push_back({ move(lineIndices), GL_LINES });
    if (!pointIndices.empty()) subMeshes.push_back({ move(pointIndices), GL_POINTS });

    // --- Create single Mesh with multiple submeshes ---
    auto meshObj = make_unique<Mesh>(vertexCount, move(subMeshes), material);
    if (!positions.empty()) meshObj->setPosition(move(positions));
    if (hasNormals)         meshObj->setNormals(move(normals));
    if (hasTexCoords)       meshObj->setTexCoords(0, move(texCoords));
    if (hasTangents) {
        meshObj->setTangents(move(tangents));
        meshObj->setBitangents(move(bitangents));
    }
    if (hasRig) {
        meshObj->setBoneIDs(move(boneIDs));
        meshObj->setWeights(move(weights));
    }

    // One interleave + one GPU upload per mesh; CPU copies are freed (the Model is read-only).
    meshObj->upload(true);

    meshes.push_back(move(meshObj));
}

const Material* Model::getOrCreateMaterial(LoadContext& ctx, unsigned int materialIndex)
{
    if (materialIndex >= ctx.materialLookup.size()) {
        return nullptr;
    }

    int32_t& lookup = ctx.materialLookup[materialIndex];
    if (lookup >= 0) {
        return m_Materials[lookup].get();
    }

    const aiScene* scene = ctx.scene;
    aiMaterial* aiMat = scene->mMaterials[materialIndex];
    auto material = make_unique<Material>();

    auto add = [&](Material::Slot slot, const vector<Texture2D*>& maps) {
        for (const Texture2D* texture : maps) {
            material->addTexture(slot, texture);
        }
        };

    add(Material::Slot::Diffuse,
        loadMaterialTextures(scene, aiMat, aiTextureType_DIFFUSE, Texture2D::Texture2DType::TextureDiffuse));
    add(Material::Slot::Specular,
        loadMaterialTextures(scene, aiMat, aiTextureType_SPECULAR, Texture2D::Texture2DType::TextureSpecular));

    // Normal map heuristic
    auto normalMaps = loadMaterialTextures(scene, aiMat, aiTextureType_NORMALS, Texture2D::Texture2DType::TextureNormal);
    if (normalMaps.empty()) {
        // fallback: Some models use height maps as normal maps
        normalMaps = loadMaterialTextures(scene, aiMat, aiTextureType_HEIGHT, Texture2D::Texture2DType::TextureNormal);
    }
    add(Material::Slot::Normal, normalMaps);

    add(Material::Slot::Height,
        loadMaterialTextures(scene, aiMat, aiTextureType_DISPLACEMENT, Texture2D::Texture2DType::TextureHeight));

    lookup = static_cast<int32_t>(m_Materials.size());
    m_Materials.push_back(move(material));
    return m_Materials.back().get();
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
    // -1 means "no bone": the vertex shader skips ids < 0 and falls back to the bind pose.
    boneIDs.fill(-1);
    weights.fill(0.0f);
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
    LoadContext& ctx,
    std::vector<std::array<int, MAX_BONE_INFLUENCE>>& boneIDs,
    std::vector<std::array<float, MAX_BONE_INFLUENCE>>& weights,
    aiMesh* mesh)
{
    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
    {
        const aiBone* bone = mesh->mBones[boneIndex];

        // Slot = index used by the vertex attribute and by the bone matrix array.
        const int boneID = ctx.rig.addBone(
            bone->mName.C_Str(),
            AssimpGLMHelpers::ConvertMatrixToGLMFormat(bone->mOffsetMatrix)
        );

        for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex)
        {
            const unsigned int vertexId = bone->mWeights[weightIndex].mVertexId;
            if (vertexId >= boneIDs.size()) {
                continue;
            }

            setVertexBoneData(boneIDs[vertexId], weights[vertexId], boneID, bone->mWeights[weightIndex].mWeight);
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