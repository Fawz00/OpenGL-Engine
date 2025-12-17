#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include "AnimationRetarget.hpp"
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
    Animation(const std::string& animationPath, Model* model, AnimationRetarget* retarget = nullptr);
    ~Animation() = default;

    // No copy, only move
    Animation(const Animation&) = delete;
    Animation& operator=(const Animation&) = delete;
    Animation(Animation&&) noexcept = default;
    Animation& operator=(Animation&&) noexcept = default;

    Bone* findBone(const std::string& name);

    inline float getTicksPerSecond() const { return m_TicksPerSecond; }
    inline float getDuration() const { return m_Duration; }
    inline const AssimpNodeData& getRootNode() const { return m_RootNode; }
    inline const std::unordered_map<std::string, BoneInfo>& getBoneIDMap() const {
        return m_BoneInfoMap;
    }

private:
    void readMissingBones(const aiAnimation* animation, Model& model);
    void readHierarchyData(AssimpNodeData& dest, const aiNode* src, const std::string& parentName = "");

    float m_Duration = 0.0f;
    float m_TicksPerSecond = 0.0f;

    std::vector<Bone> m_Bones;
    AssimpNodeData m_RootNode;
    std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;
    std::unordered_map<std::string, std::string> m_NodeParentMap;

	AnimationRetarget* m_Retarget = nullptr;
};
