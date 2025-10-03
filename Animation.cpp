#include "Animation.hpp"

#include "AssimpGLMHelpers.hpp"
#include "Debug.hpp"
#include <cassert>
#include <algorithm>

Animation::Animation(const std::string& animationPath, Model* model) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        Debug::logError("ERROR::ASSIMP:: " + std::string(importer.GetErrorString()));
        return;
    }

    if (!scene->HasAnimations()) {
        Debug::logError("ERROR::ANIMATION:: No animations found in " + animationPath);
        return;
    }

    const aiAnimation* animation = scene->mAnimations[0];
    m_Duration = static_cast<float>(animation->mDuration);
    m_TicksPerSecond = (animation->mTicksPerSecond != 0.0f)
        ? static_cast<float>(animation->mTicksPerSecond)
        : 25.0f; // fallback

    m_Bones.reserve(animation->mNumChannels);
    readHierarchyData(m_RootNode, scene->mRootNode);
    readMissingBones(animation, *model);
}

Bone* Animation::findBone(const std::string& name) {
    auto it = std::find_if(m_Bones.begin(), m_Bones.end(),
        [&](const Bone& bone) { return bone.GetBoneName() == name; });

    return (it != m_Bones.end()) ? &(*it) : nullptr;
}

void Animation::readMissingBones(const aiAnimation* animation, Model& model) {
    auto& boneInfoMap = model.getBoneInfoMap();
    int& boneCount = model.getBoneCount();

    for (unsigned int i = 0; i < animation->mNumChannels; ++i) {
        aiNodeAnim* channel = animation->mChannels[i];
        std::string boneName(channel->mNodeName.data);

		// Add new bone to the map if it doesn't exist
        if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
            boneInfoMap[boneName].id = boneCount++;
            boneInfoMap[boneName].offset = glm::mat4(1.0f);
        }

        m_Bones.emplace_back(boneName, boneInfoMap[boneName].id, channel);
    }

	// Copy the bone info map from the model to the animation
    m_BoneInfoMap = boneInfoMap;
}

void Animation::readHierarchyData(AssimpNodeData& dest, const aiNode* src) {
    if (!src) return;

    dest.name = src->mName.C_Str();
    dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
    dest.childrenCount = static_cast<int>(src->mNumChildren);
    dest.children.reserve(src->mNumChildren);

    for (unsigned int i = 0; i < src->mNumChildren; ++i) {
        AssimpNodeData child;
        readHierarchyData(child, src->mChildren[i]);
        dest.children.push_back(std::move(child));
    }
}
