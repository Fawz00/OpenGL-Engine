#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

struct aiNode;

/**
 * Rig: immutable skeleton data extracted from a model file.
 *
 * What it stores
 * --------------
 *  - A flattened node hierarchy in PRE-ORDER (a parent always has a smaller
 *    index than its children). Only nodes that are a bone, or have a bone
 *    among their descendants, are kept: meshes/lights/cameras nodes are
 *    pruned because they never influence skinning.
 *  - The bind-pose local transform of every kept node.
 *  - For every bone: its "slot" (the index used by the vertex attribute
 *    `boneIds` and by the `finalBonesMatrices[]` uniform array) and its
 *    offset (inverse bind) matrix.
 *
 * Why pre-order + flat arrays
 * ---------------------------
 * Evaluating a pose becomes ONE linear loop (`global[i] = global[parent[i]] * local[i]`)
 * with no recursion, no pointer chasing and no string lookups.
 *
 * A Rig can only be created through Rig::Builder (done by Model while importing),
 * and is read-only afterwards.
 */
class Rig {
public:
    static constexpr int32_t NoIndex = -1;

    /**
     * Collects bones while the meshes of a model are being imported, then
     * produces the final immutable Rig with build().
     * Bone slots are assigned in first-seen order.
     */
    class Builder {
    public:
        /**
         * Registers a bone and returns its slot. If a bone with this name is
         * already registered, the existing slot is returned and the new offset
         * is ignored (first one wins).
         */
        int32_t addBone(const std::string& name, const glm::mat4& offset);

        /**
         * Returns the slot of the bone named after the scene root node,
         * creating it (identity offset) if it does not exist yet.
         * Used by ConfigOption::Force when vertices have no bone weights.
         */
        int32_t ensureRootBone(const std::string& rootNodeName);

        /** Returns the slot of a bone, or Rig::NoIndex if it is not registered. */
        [[nodiscard]] int32_t findBone(const std::string& name) const;

        [[nodiscard]] int32_t getBoneCount() const noexcept { return static_cast<int32_t>(m_BoneNames.size()); }

        /**
         * Builds the Rig from the registered bones and the scene hierarchy.
         * Returns an empty Rig when no bone was registered or root is null.
         * Bones that have no matching node are kept (their matrix stays identity)
         * and a warning is logged.
         */
        [[nodiscard]] Rig build(const aiNode* root) const;

    private:
        std::vector<std::string> m_BoneNames;
        std::vector<glm::mat4> m_BoneOffsets;
        std::unordered_map<std::string, int32_t> m_BoneIndex;
    };

    Rig() = default;

    /** True when the rig has no node (model without skeleton). */
    [[nodiscard]] bool empty() const noexcept { return m_NodeNames.empty(); }

    // ---- Counts ----------------------------------------------------------
    [[nodiscard]] int32_t getNodeCount() const noexcept { return static_cast<int32_t>(m_NodeNames.size()); }
    [[nodiscard]] int32_t getBoneCount() const noexcept { return static_cast<int32_t>(m_BoneNames.size()); }

    // ---- Lookup (load/bind time only, NOT for the per-frame loop) --------
    /** Node index by name, or NoIndex. */
    [[nodiscard]] int32_t findNode(const std::string& name) const;
    /** Bone slot by name, or NoIndex. */
    [[nodiscard]] int32_t findBone(const std::string& name) const;

    // ---- Per node --------------------------------------------------------
    [[nodiscard]] const std::string& getNodeName(int32_t node) const { return m_NodeNames[node]; }
    [[nodiscard]] int32_t getNodeParent(int32_t node) const { return m_NodeParent[node]; }
    [[nodiscard]] const glm::mat4& getNodeBindLocal(int32_t node) const { return m_NodeBindLocal[node]; }
    /** Bone slot of a node, or NoIndex when the node is only a hierarchy ancestor. */
    [[nodiscard]] int32_t getNodeBoneSlot(int32_t node) const { return m_NodeBoneSlot[node]; }

    // ---- Per bone --------------------------------------------------------
    [[nodiscard]] const std::string& getBoneName(int32_t slot) const { return m_BoneNames[slot]; }
    [[nodiscard]] const glm::mat4& getBoneOffset(int32_t slot) const { return m_BoneOffsets[slot]; }
    /** Node index of a bone, or NoIndex when the bone has no node in the hierarchy. */
    [[nodiscard]] int32_t getBoneNode(int32_t slot) const { return m_BoneNode[slot]; }

    [[nodiscard]] const std::string& getRootName() const noexcept { return m_RootName; }

    // ---- Raw arrays for hot loops ---------------------------------------
    [[nodiscard]] std::span<const int32_t> getNodeParents() const noexcept { return m_NodeParent; }
    [[nodiscard]] std::span<const int32_t> getNodeBoneSlots() const noexcept { return m_NodeBoneSlot; }
    [[nodiscard]] std::span<const glm::mat4> getNodeBindLocals() const noexcept { return m_NodeBindLocal; }
    [[nodiscard]] std::span<const glm::mat4> getBoneOffsets() const noexcept { return m_BoneOffsets; }

private:
    // Node data (pre-order, parent index < child index)
    std::vector<std::string> m_NodeNames;
    std::vector<int32_t> m_NodeParent;
    std::vector<glm::mat4> m_NodeBindLocal;
    std::vector<int32_t> m_NodeBoneSlot;
    std::unordered_map<std::string, int32_t> m_NodeIndexByName;

    // Bone data (indexed by slot)
    std::vector<std::string> m_BoneNames;
    std::vector<glm::mat4> m_BoneOffsets;
    std::vector<int32_t> m_BoneNode;

    std::string m_RootName;
};
