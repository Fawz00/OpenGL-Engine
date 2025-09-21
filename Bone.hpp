#pragma once

/* Container for bone data */

#include <vector>
#include <assimp/scene.h>
#include <list>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include "AssimpGLMHelpers.hpp"

struct KeyPosition {
    glm::vec3 position;
    float timeStamp;
};

struct KeyRotation {
    glm::quat orientation;
    float timeStamp;
};

struct KeyScale {
    glm::vec3 scale;
    float timeStamp;
};

class Bone {
public:
    Bone(const std::string& name, int ID, const aiNodeAnim* channel)
        : m_Name(name), m_ID(ID), m_LocalTransform(1.0f)
    {
        if (!channel) return;

        m_Positions.reserve(channel->mNumPositionKeys);
        for (unsigned int i = 0; i < channel->mNumPositionKeys; ++i) {
            aiVector3D aiPos = channel->mPositionKeys[i].mValue;
            float t = static_cast<float>(channel->mPositionKeys[i].mTime);
            m_Positions.push_back({ AssimpGLMHelpers::GetGLMVec(aiPos), t });
        }

        m_Rotations.reserve(channel->mNumRotationKeys);
        for (unsigned int i = 0; i < channel->mNumRotationKeys; ++i) {
            aiQuaternion aiRot = channel->mRotationKeys[i].mValue;
            float t = static_cast<float>(channel->mRotationKeys[i].mTime);
            m_Rotations.push_back({ glm::normalize(AssimpGLMHelpers::GetGLMQuat(aiRot)), t });
        }

        m_Scales.reserve(channel->mNumScalingKeys);
        for (unsigned int i = 0; i < channel->mNumScalingKeys; ++i) {
            aiVector3D aiScale = channel->mScalingKeys[i].mValue;
            float t = static_cast<float>(channel->mScalingKeys[i].mTime);
            m_Scales.push_back({ AssimpGLMHelpers::GetGLMVec(aiScale), t });
        }

        m_NumPositions = static_cast<int>(m_Positions.size());
        m_NumRotations = static_cast<int>(m_Rotations.size());
        m_NumScalings = static_cast<int>(m_Scales.size());
    }

    void Update(float animationTime) {
        glm::mat4 translation = InterpolatePosition(animationTime);
        glm::mat4 rotation = InterpolateRotation(animationTime);
        glm::mat4 scale = InterpolateScaling(animationTime);
        m_LocalTransform = translation * rotation * scale;
    }

    [[nodiscard]] inline glm::mat4 GetLocalTransform() const noexcept { return m_LocalTransform; }
    [[nodiscard]] inline const std::string& GetBoneName() const noexcept { return m_Name; }
    [[nodiscard]] inline int GetBoneID() const noexcept { return m_ID; }

private:
    // ---- Optimized keyframe search (binary search) ----
    int GetPositionIndex(float time) const {
        auto it = std::upper_bound(m_Positions.begin(), m_Positions.end(), time,
            [](float t, const KeyPosition& kp) { return t < kp.timeStamp; });
        int idx = static_cast<int>(std::max(0, static_cast<int>(it - m_Positions.begin()) - 1));
        return idx;
    }

    int GetRotationIndex(float time) const {
        auto it = std::upper_bound(m_Rotations.begin(), m_Rotations.end(), time,
            [](float t, const KeyRotation& kr) { return t < kr.timeStamp; });
        int idx = static_cast<int>(std::max(0, static_cast<int>(it - m_Rotations.begin()) - 1));
        return idx;
    }

    int GetScaleIndex(float time) const {
        auto it = std::upper_bound(m_Scales.begin(), m_Scales.end(), time,
            [](float t, const KeyScale& ks) { return t < ks.timeStamp; });
        int idx = static_cast<int>(std::max(0, static_cast<int>(it - m_Scales.begin()) - 1));
        return idx;
    }

    // ---- Interpolation helpers ----
    static inline float GetScaleFactor(float lastTime, float nextTime, float time) noexcept {
        float length = nextTime - lastTime;
        if (length < 1e-6f) return 0.0f;
        return (time - lastTime) / length;
    }

    glm::mat4 InterpolatePosition(float time) const {
        if (m_NumPositions == 0) return glm::mat4(1.0f);
        if (m_NumPositions == 1) return glm::translate(glm::mat4(1.0f), m_Positions[0].position);

        int idx = GetPositionIndex(time);
        int nextIdx = std::min(idx + 1, m_NumPositions - 1);

        float factor = GetScaleFactor(m_Positions[idx].timeStamp, m_Positions[nextIdx].timeStamp, time);
        glm::vec3 finalPos = glm::mix(m_Positions[idx].position, m_Positions[nextIdx].position, factor);
        return glm::translate(glm::mat4(1.0f), finalPos);
    }

    glm::mat4 InterpolateRotation(float time) const {
        if (m_NumRotations == 0) return glm::mat4(1.0f);
        if (m_NumRotations == 1) return glm::toMat4(glm::normalize(m_Rotations[0].orientation));

        int idx = GetRotationIndex(time);
        int nextIdx = std::min(idx + 1, m_NumRotations - 1);

        float factor = GetScaleFactor(m_Rotations[idx].timeStamp, m_Rotations[nextIdx].timeStamp, time);
        glm::quat finalRot = glm::slerp(m_Rotations[idx].orientation, m_Rotations[nextIdx].orientation, factor);
        return glm::toMat4(glm::normalize(finalRot));
    }

    glm::mat4 InterpolateScaling(float time) const {
        if (m_NumScalings == 0) return glm::mat4(1.0f);
        if (m_NumScalings == 1) return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);

        int idx = GetScaleIndex(time);
        int nextIdx = std::min(idx + 1, m_NumScalings - 1);

        float factor = GetScaleFactor(m_Scales[idx].timeStamp, m_Scales[nextIdx].timeStamp, time);
        glm::vec3 finalScale = glm::mix(m_Scales[idx].scale, m_Scales[nextIdx].scale, factor);
        return glm::scale(glm::mat4(1.0f), finalScale);
    }

    // ---- Data ----
    std::vector<KeyPosition> m_Positions;
    std::vector<KeyRotation> m_Rotations;
    std::vector<KeyScale>    m_Scales;

    int m_NumPositions = 0;
    int m_NumRotations = 0;
    int m_NumScalings = 0;

    glm::mat4 m_LocalTransform{ 1.0f };
    std::string m_Name;
    int m_ID = -1;
};
