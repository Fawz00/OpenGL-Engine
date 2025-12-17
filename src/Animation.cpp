#include "Animation.hpp"

#include "AssimpGLMHelpers.hpp"
#include "Debug.hpp"
#include <cassert>
#include <algorithm>

Animation::Animation(const std::string& animationPath, Model* model, AnimationRetarget* retarget)
    : m_Retarget(retarget)
    {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(animationPath,
        aiProcess_Triangulate
    );

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
        std::string sourceBoneName(channel->mNodeName.data);
        std::string boneName = sourceBoneName;

        // Apply retarget mapping
        if (m_Retarget) {
            std::string targetBone = m_Retarget->mapBoneName(sourceBoneName);
            if (!targetBone.empty()) {
                boneName = targetBone;
            }
            else {
                Debug::logWarn("ANIMATION: Skipping bone (not in retarget map): " + sourceBoneName);
                continue; // strict mode
            }
        }

        // If the bone is not in the model, add it
        if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
            std::string fallbackParent = m_NodeParentMap[sourceBoneName]; // try original name first
            while (!fallbackParent.empty() && boneInfoMap.find(fallbackParent) == boneInfoMap.end()) {
                fallbackParent = m_NodeParentMap[fallbackParent];
            }

            Debug::logWarn("ANIMATION: Bone " + boneName +
                " not found in model. Adding it with fallback parent " + fallbackParent);

            boneInfoMap[boneName].id = boneCount++;
            boneInfoMap[boneName].offset = glm::mat4(1.0f);
        }

        m_Bones.emplace_back(boneName, boneInfoMap[boneName].id, channel);
    }

	// Copy the bone info map from the model to the animation
    m_BoneInfoMap = boneInfoMap;
}

void Animation::readHierarchyData(AssimpNodeData& dest, const aiNode* src, const std::string& parentName) {
    if (!src) return;

    std::string sourceName = src->mName.C_Str();
    std::string targetName = sourceName;

    // Apply retarget mapping: source -> target
    if (m_Retarget) {
        std::string mappedName = m_Retarget->mapBoneName(sourceName); // source -> target
        if (!mappedName.empty()) {
            targetName = mappedName;
        }
    }

    dest.name = targetName;
    dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
    dest.childrenCount = static_cast<int>(src->mNumChildren);
    dest.children.reserve(src->mNumChildren);

	// Record parent mapping
    std::string mappedParent = parentName;
    if (!parentName.empty()) {
        mappedParent = m_NodeParentMap[parentName];
    }
    m_NodeParentMap[targetName] = mappedParent;

    for (unsigned int i = 0; i < src->mNumChildren; ++i) {
        AssimpNodeData child;
        readHierarchyData(child, src->mChildren[i], targetName);
        dest.children.push_back(std::move(child));
    }
}
