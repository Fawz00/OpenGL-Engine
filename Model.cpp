#include "Model.hpp"

Model::Model(string const& path, bool gamma)
    : gammaCorrection(gamma)
{
    loadModel(path);
}

Model::~Model()
{
    // Cleanup textures
    for (auto& texture : textures_loaded) {
        delete texture;
    }

    // Cleanup meshes
    for (auto& mesh : meshes) {
        delete mesh;
    }
}

void Model::Draw(Shader& shader)
{
    for (unsigned int i = 0; i < meshes.size(); i++) {
        meshes[i]->Draw(shader);
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

void Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    vector<Vertex> vertices;
    vector<unsigned int> triIndices;
    vector<unsigned int> lineIndices;
    vector<unsigned int> pointIndices;
    vector<Texture2D*> textures;

    // --- Vertex data ---
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        glm::vec3 vector;

        // Positions
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;

        // Normals
        if (mesh->HasNormals()) {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }

        // Texture coordinates
        if (mesh->mTextureCoords[0]) {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;

            // Tangent
            if (mesh->mTangents) {
                vertex.Tangent = glm::vec3(
                    mesh->mTangents[i].x,
                    mesh->mTangents[i].y,
                    mesh->mTangents[i].z
                );
            }

            // Bitangent
            if (mesh->mBitangents) {
                vertex.Bitangent = glm::vec3(
                    mesh->mBitangents[i].x,
                    mesh->mBitangents[i].y,
                    mesh->mBitangents[i].z
                );
            }
        }
        else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    // --- Indices ---
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];

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
		// If the face has more than 3 indices, we assume it's a polygon and triangulate it
    }

    // --- Textures ---
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    // 1. Diffuse maps
    vector<Texture2D*> diffuseMaps = loadMaterialTextures(scene, material, aiTextureType_DIFFUSE, Texture2D::TextureType::TextureDiffuse);
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

    // 2. Specular maps
    vector<Texture2D*> specularMaps = loadMaterialTextures(scene, material, aiTextureType_SPECULAR, Texture2D::TextureType::TextureSpecular);
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    // 3. Normal maps
    vector<Texture2D*> normalMaps = loadMaterialTextures(scene, material, aiTextureType_HEIGHT, Texture2D::TextureType::TextureNormal);
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

    // 4. Height maps
    vector<Texture2D*> heightMaps = loadMaterialTextures(scene, material, aiTextureType_AMBIENT, Texture2D::TextureType::TextureHeight);
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    // --- Create Mesh objects ---
	// Split into separate meshes based on primitive types
    if (!triIndices.empty()) {
        meshes.push_back(new Mesh(vertices, triIndices, textures, GL_TRIANGLES));
    }
    if (!lineIndices.empty()) {
        meshes.push_back(new Mesh(vertices, lineIndices, textures, GL_LINES));
    }
    if (!pointIndices.empty()) {
        meshes.push_back(new Mesh(vertices, pointIndices, textures, GL_POINTS));
    }
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

        if (str.C_Str()[0] == '*') {
            // Embedded texture
            int texIndex = atoi(str.C_Str() + 1);
            aiTexture* aiTex = scene->mTextures[texIndex];

            if (aiTex->mHeight == 0) {
                // Compressed format (jpg/png)
                unsigned char* data = reinterpret_cast<unsigned char*>(aiTex->pcData);
				size_t size = aiTex->mWidth; // size of the compressed texture data in bytes

                Texture2D* texture = new Texture2D(data, size, texType);
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
            else {
                // Raw RGBA (uncompressed)
                unsigned char* data = reinterpret_cast<unsigned char*>(aiTex->pcData);
                int width = aiTex->mWidth;
                int height = aiTex->mHeight;
                int channels = 4;

                Texture2D* texture = new Texture2D(data, width, height, channels, texType);
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        else {
			// Texture from file
            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++) {
                if (std::strcmp(textures_loaded[j]->getPath().data(), str.C_Str()) == 0) {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            if (!skip) {
                Texture2D* texture = new Texture2D("Resources/engine/textures/" + string(str.C_Str()), true, texType);
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
    }

    return textures;
}
