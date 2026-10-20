#include "Animation.hpp"

#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <unordered_map>

#include "AssimpGLMHelpers.hpp"
#include "Debug.hpp"

Animation::Animation(const std::string& animationPath, AnimationSourceFormat format)
    : m_Path(animationPath),
    m_Format(format == AnimationSourceFormat::Auto ? detectAnimationFormat(animationPath) : format)
{
    const AnimationFormatPolicy& policy = getAnimationFormatPolicy(m_Format);

    Assimp::Importer importer;

    // Only the animation timelines are wanted: skip data that costs parse time and is irrelevant here.
    if (policy.applyFbxReaderOptions) {
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_MATERIALS, false);
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_TEXTURES, false);
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_CAMERAS, false);
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_LIGHTS, false);
    }

    // No post-processing: meshes are not used, so triangulation etc. would be wasted work.
    const aiScene* scene = importer.ReadFile(animationPath, 0);

    // NOTE: AI_SCENE_FLAGS_INCOMPLETE is NOT treated as an error here. Animation-only files
    // (no meshes) are flagged incomplete by Assimp but are perfectly valid for this class.
    if (!scene || !scene->mRootNode) {
        Debug::logError("ERROR::ASSIMP:: " + std::string(importer.GetErrorString()));
        return;
    }

    if (!scene->HasAnimations()) {
        Debug::logError("ERROR::ANIMATION:: No animations found in " + animationPath);
        return;
    }

    // ---- 1) Flatten the file hierarchy (pre-order, iterative) -------------
    struct FlatNode {
        const aiNode* node;
        int32_t parent;
    };
    std::vector<FlatNode> flat;
    flat.reserve(128);
    {
        std::vector<FlatNode> stack;
        stack.push_back({ scene->mRootNode, -1 });
        while (!stack.empty()) {
            const FlatNode current = stack.back();
            stack.pop_back();

            const int32_t index = static_cast<int32_t>(flat.size());
            flat.push_back(current);
            for (unsigned int c = current.node->mNumChildren; c-- > 0;) {
                stack.push_back({ current.node->mChildren[c], index });
            }
        }
    }

    std::unordered_map<std::string, int32_t> flatIndexByName;
    flatIndexByName.reserve(flat.size());
    for (size_t i = 0; i < flat.size(); ++i) {
        flatIndexByName.try_emplace(flat[i].node->mName.C_Str(), static_cast<int32_t>(i)); // first wins
    }

    // ---- 2) Keep only channel targets and their ancestors -----------------
    std::vector<uint8_t> needed(flat.size(), 0);
    for (unsigned int a = 0; a < scene->mNumAnimations; ++a) {
        const aiAnimation* anim = scene->mAnimations[a];
        for (unsigned int c = 0; c < anim->mNumChannels; ++c) {
            auto it = flatIndexByName.find(anim->mChannels[c]->mNodeName.C_Str());
            if (it != flatIndexByName.end()) needed[it->second] = 1;
        }
    }
    for (size_t i = flat.size(); i-- > 0;) {
        if (needed[i] && flat[i].parent >= 0) needed[flat[i].parent] = 1;
    }

    std::vector<int32_t> remap(flat.size(), -1);
    int32_t kept = 0;
    for (size_t i = 0; i < flat.size(); ++i) {
        if (needed[i]) remap[i] = kept++;
    }

    m_Nodes.reserve(kept);
    for (size_t i = 0; i < flat.size(); ++i) {
        if (!needed[i]) continue;
        AnimationNode node;
        node.name = flat[i].node->mName.C_Str();
        node.parent = flat[i].parent < 0 ? -1 : remap[flat[i].parent];
        node.local = AssimpGLMHelpers::ConvertMatrixToGLMFormat(flat[i].node->mTransformation);
        m_Nodes.push_back(std::move(node));
    }

    // ---- 3) Clips ----------------------------------------------------------
    m_Clips.reserve(scene->mNumAnimations);
    for (unsigned int a = 0; a < scene->mNumAnimations; ++a) {
        const aiAnimation* anim = scene->mAnimations[a];

        AnimationClip clip;
        clip.m_Name = anim->mName.length > 0 ? std::string(anim->mName.C_Str()) : "clip_" + std::to_string(a);
        clip.m_Duration = static_cast<float>(anim->mDuration);
        clip.m_TicksPerSecond = anim->mTicksPerSecond != 0.0
            ? static_cast<float>(anim->mTicksPerSecond)
            : policy.fallbackTicksPerSecond;

        // Reserve exact sizes: one allocation per array instead of repeated growth.
        size_t posKeys = 0, rotKeys = 0, sclKeys = 0;
        for (unsigned int c = 0; c < anim->mNumChannels; ++c) {
            const aiNodeAnim* ch = anim->mChannels[c];
            posKeys += ch->mNumPositionKeys ? ch->mNumPositionKeys : 1;
            rotKeys += ch->mNumRotationKeys ? ch->mNumRotationKeys : 1;
            sclKeys += ch->mNumScalingKeys ? ch->mNumScalingKeys : 1;
        }
        clip.m_PositionTimes.reserve(posKeys);
        clip.m_PositionValues.reserve(posKeys);
        clip.m_RotationTimes.reserve(rotKeys);
        clip.m_RotationValues.reserve(rotKeys);
        clip.m_ScaleTimes.reserve(sclKeys);
        clip.m_ScaleValues.reserve(sclKeys);
        clip.m_Channels.reserve(anim->mNumChannels);

        unsigned int missingNodes = 0;

        for (unsigned int c = 0; c < anim->mNumChannels; ++c) {
            const aiNodeAnim* src = anim->mChannels[c];

            AnimationChannel channel;
            channel.nodeName = src->mNodeName.C_Str();

            // Static transform of the node, decomposed. Used when a track has no keys:
            // a missing track must keep the node's own value (identity would collapse bones).
            aiVector3D staticScale(1.0f, 1.0f, 1.0f), staticPos(0.0f, 0.0f, 0.0f);
            aiQuaternion staticRot;
            auto it = flatIndexByName.find(channel.nodeName);
            if (it != flatIndexByName.end()) {
                channel.nodeIndex = remap[it->second];
                flat[it->second].node->mTransformation.Decompose(staticScale, staticRot, staticPos);
            }
            else {
                ++missingNodes;
            }

            // Position
            channel.position.offset = static_cast<uint32_t>(clip.m_PositionTimes.size());
            if (src->mNumPositionKeys > 0) {
                for (unsigned int k = 0; k < src->mNumPositionKeys; ++k) {
                    clip.m_PositionTimes.push_back(static_cast<float>(src->mPositionKeys[k].mTime));
                    clip.m_PositionValues.push_back(AssimpGLMHelpers::GetGLMVec(src->mPositionKeys[k].mValue));
                }
                channel.position.count = src->mNumPositionKeys;
            }
            else {
                clip.m_PositionTimes.push_back(0.0f);
                clip.m_PositionValues.push_back(AssimpGLMHelpers::GetGLMVec(staticPos));
                channel.position.count = 1;
            }

            // Rotation (normalized once here, not per frame)
            channel.rotation.offset = static_cast<uint32_t>(clip.m_RotationTimes.size());
            if (src->mNumRotationKeys > 0) {
                for (unsigned int k = 0; k < src->mNumRotationKeys; ++k) {
                    clip.m_RotationTimes.push_back(static_cast<float>(src->mRotationKeys[k].mTime));
                    clip.m_RotationValues.push_back(
                        glm::normalize(AssimpGLMHelpers::GetGLMQuat(src->mRotationKeys[k].mValue)));
                }
                channel.rotation.count = src->mNumRotationKeys;
            }
            else {
                clip.m_RotationTimes.push_back(0.0f);
                clip.m_RotationValues.push_back(glm::normalize(AssimpGLMHelpers::GetGLMQuat(staticRot)));
                channel.rotation.count = 1;
            }

            // Scale
            channel.scale.offset = static_cast<uint32_t>(clip.m_ScaleTimes.size());
            if (src->mNumScalingKeys > 0) {
                for (unsigned int k = 0; k < src->mNumScalingKeys; ++k) {
                    clip.m_ScaleTimes.push_back(static_cast<float>(src->mScalingKeys[k].mTime));
                    clip.m_ScaleValues.push_back(AssimpGLMHelpers::GetGLMVec(src->mScalingKeys[k].mValue));
                }
                channel.scale.count = src->mNumScalingKeys;
            }
            else {
                clip.m_ScaleTimes.push_back(0.0f);
                clip.m_ScaleValues.push_back(AssimpGLMHelpers::GetGLMVec(staticScale));
                channel.scale.count = 1;
            }

            channel.constant = channel.position.count <= 1
                && channel.rotation.count <= 1
                && channel.scale.count <= 1;

            clip.m_Channels.push_back(std::move(channel));
        }

        if (missingNodes > 0) {
            Debug::logWarn("ANIMATION: clip '" + clip.m_Name + "' has " + std::to_string(missingNodes)
                + " channel(s) targeting nodes that do not exist in " + animationPath + ". They will be ignored.");
        }

        m_Clips.push_back(std::move(clip));
    }
}

int32_t Animation::findClip(std::string_view name) const
{
    for (size_t i = 0; i < m_Clips.size(); ++i) {
        if (m_Clips[i].getName() == name) return static_cast<int32_t>(i);
    }
    return -1;
}