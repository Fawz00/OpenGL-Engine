#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include "Bone.hpp"
#include "Model.hpp"

struct AssimpNodeData {
    glm::mat4 transformation;
    std::string name;
    int childrenCount = 0;
    std::vector<AssimpNodeData> children;
};

class Animation {
public:
    Animation() = default;
    Animation(const std::string& animationPath, Model* model);
    ~Animation() = default;

    // No copy, only move
    Animation(const Animation&) = delete;
    Animation& operator=(const Animation&) = delete;
    Animation(Animation&&) noexcept = default;
    Animation& operator=(Animation&&) noexcept = default;

    Bone* FindBone(const std::string& name);

    inline float GetTicksPerSecond() const { return m_TicksPerSecond; }
    inline float GetDuration() const { return m_Duration; }
    inline const AssimpNodeData& GetRootNode() const { return m_RootNode; }
    inline const std::unordered_map<std::string, BoneInfo>& GetBoneIDMap() const {
        return m_BoneInfoMap;
    }

private:
    void ReadMissingBones(const aiAnimation* animation, Model& model);
    void ReadHierarchyData(AssimpNodeData& dest, const aiNode* src);

    float m_Duration = 0.0f;
    float m_TicksPerSecond = 0.0f;

    std::vector<Bone> m_Bones;
    AssimpNodeData m_RootNode;
    std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;
};
