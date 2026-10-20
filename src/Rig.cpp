#include "Rig.hpp"

#include <assimp/scene.h>
#include <utility>

#include "AssimpGLMHelpers.hpp"
#include "Debug.hpp"

// ============================================================================
// Rig::Builder
// ============================================================================

int32_t Rig::Builder::addBone(const std::string& name, const glm::mat4& offset)
{
    auto [it, inserted] = m_BoneIndex.try_emplace(name, static_cast<int32_t>(m_BoneNames.size()));
    if (inserted) {
        m_BoneNames.push_back(name);
        m_BoneOffsets.push_back(offset);
    }
    return it->second;
}

int32_t Rig::Builder::ensureRootBone(const std::string& rootNodeName)
{
    const int32_t existing = findBone(rootNodeName);
    if (existing != NoIndex) {
        return existing;
    }

    const int32_t slot = addBone(rootNodeName, glm::mat4(1.0f));
    Debug::logWarn("Rig: no bone weights found for some vertices. Created a root bone named '"
        + rootNodeName + "' with slot " + std::to_string(slot) + ".");
    return slot;
}

int32_t Rig::Builder::findBone(const std::string& name) const
{
    auto it = m_BoneIndex.find(name);
    return it != m_BoneIndex.end() ? it->second : NoIndex;
}

Rig Rig::Builder::build(const aiNode* root) const
{
    Rig rig;
    if (!root || m_BoneNames.empty()) {
        return rig;
    }

    // 1) Flatten the whole scene hierarchy in pre-order (iterative, no recursion).
    //    Children are pushed in reverse so they are popped in their original order.
    struct FlatNode {
        const aiNode* node;
        int32_t parent;
    };
    std::vector<FlatNode> flat;
    flat.reserve(128);

    std::vector<FlatNode> stack;
    stack.push_back({ root, NoIndex });
    while (!stack.empty()) {
        const FlatNode current = stack.back();
        stack.pop_back();

        const int32_t index = static_cast<int32_t>(flat.size());
        flat.push_back(current);

        for (unsigned int c = current.node->mNumChildren; c-- > 0;) {
            stack.push_back({ current.node->mChildren[c], index });
        }
    }

    // 2) Mark bones, then mark all their ancestors (pre-order => parent index < child index,
    //    so a single reverse pass propagates "needed" up to the root).
    const size_t flatCount = flat.size();
    std::vector<uint8_t> needed(flatCount, 0);
    std::vector<int32_t> flatBone(flatCount, NoIndex);

    for (size_t i = 0; i < flatCount; ++i) {
        auto it = m_BoneIndex.find(flat[i].node->mName.C_Str());
        if (it != m_BoneIndex.end()) {
            flatBone[i] = it->second;
            needed[i] = 1;
        }
    }
    for (size_t i = flatCount; i-- > 0;) {
        if (needed[i] && flat[i].parent != NoIndex) {
            needed[flat[i].parent] = 1;
        }
    }

    // 3) Compact into the final arrays (order is preserved, so pre-order still holds).
    std::vector<int32_t> remap(flatCount, NoIndex);
    int32_t kept = 0;
    for (size_t i = 0; i < flatCount; ++i) {
        if (needed[i]) remap[i] = kept++;
    }

    rig.m_NodeNames.reserve(kept);
    rig.m_NodeParent.reserve(kept);
    rig.m_NodeBindLocal.reserve(kept);
    rig.m_NodeBoneSlot.reserve(kept);
    rig.m_NodeIndexByName.reserve(kept);

    rig.m_BoneNames = m_BoneNames;
    rig.m_BoneOffsets = m_BoneOffsets;
    rig.m_BoneNode.assign(m_BoneNames.size(), NoIndex);
    rig.m_RootName = root->mName.C_Str();

    for (size_t i = 0; i < flatCount; ++i) {
        if (!needed[i]) continue;

        const int32_t nodeIndex = remap[i];
        const aiNode* node = flat[i].node;
        std::string name = node->mName.C_Str();

        int32_t slot = flatBone[i];
        if (slot != NoIndex) {
            if (rig.m_BoneNode[slot] == NoIndex) {
                rig.m_BoneNode[slot] = nodeIndex;
            }
            else {
                Debug::logWarn("Rig: duplicate node name '" + name + "'. Only the first node drives bone slot "
                    + std::to_string(slot) + ".");
                slot = NoIndex;
            }
        }

        const int32_t parent = flat[i].parent == NoIndex ? NoIndex : remap[flat[i].parent];

        rig.m_NodeParent.push_back(parent);
        rig.m_NodeBindLocal.push_back(AssimpGLMHelpers::ConvertMatrixToGLMFormat(node->mTransformation));
        rig.m_NodeBoneSlot.push_back(slot);
        rig.m_NodeIndexByName.try_emplace(name, nodeIndex); // first one wins
        rig.m_NodeNames.push_back(std::move(name));
    }

    for (size_t b = 0; b < rig.m_BoneNode.size(); ++b) {
        if (rig.m_BoneNode[b] == NoIndex) {
            Debug::logWarn("Rig: bone '" + rig.m_BoneNames[b] + "' has no node in the hierarchy. Its matrix stays identity.");
        }
    }

    return rig;
}

// ============================================================================
// Rig
// ============================================================================

int32_t Rig::findNode(const std::string& name) const
{
    auto it = m_NodeIndexByName.find(name);
    return it != m_NodeIndexByName.end() ? it->second : NoIndex;
}

int32_t Rig::findBone(const std::string& name) const
{
    const int32_t node = findNode(name);
    return node != NoIndex ? m_NodeBoneSlot[node] : NoIndex;
}
