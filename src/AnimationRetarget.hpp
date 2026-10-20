#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include <nlohmann/json.hpp>

/**
 * AnimationRetarget: name mapping from the node names used in an animation file
 * (source) to the node names used in the model Rig (target).
 *
 * It is a pure name map (no rest-pose correction). It is applied ONCE, when an
 * Animator binds an animation (Animator::play), never per frame.
 *
 * JSON format: { "sourceName": "targetName", ... }
 *
 * Modes (what happens to a name that is not in the map):
 *  - Strict   : the node is NOT mapped. Its channel is ignored and, if it sits between
 *               two mapped nodes, its static transform from the animation file is used.
 *  - Flexible : the node keeps its own name (identity mapping).
 *
 * An entry with an empty target string means "explicitly skip this node" in both modes.
 *
 * Pivot (helper) nodes
 * --------------------
 * Some importers split a node into a chain of helper nodes named "<node><tag><Kind>" (FBX:
 * "Hips_$AssimpFbx$_Rotation"), and the animated channels usually live on those helpers.
 * You do NOT need to list them: when the caller passes the format's pivot tag (see
 * AnimationFormat.hpp), a helper follows the mapping of its base node, i.e.
 *     "mixamorig:Hips_$AssimpFbx$_Rotation"  ->  map["mixamorig:Hips"] + "_$AssimpFbx$_Rotation"
 * An explicit entry for the full helper name always wins over the derived one. Helpers of a base
 * node that is not mapped are treated like any other unmapped node (see modes above).
 * For formats without such nodes the tag is empty and this step is skipped.
 */
class AnimationRetarget {
public:
    enum class Mode {
        Flexible,
        Strict
    };

    AnimationRetarget() = default;
    explicit AnimationRetarget(const std::string& jsonPath, Mode mode = Mode::Strict);

    /** Loads (replaces) the map from a JSON file. Returns false and logs on failure. */
    bool loadFromFile(const std::string& jsonPath);
    [[nodiscard]] bool isLoaded() const noexcept { return m_Loaded; }

    void setMode(Mode mode);
    [[nodiscard]] Mode getMode() const;

    /**
     * Resolves a source name according to the current mode.
     * @param pivotTag pivot marker of the animation's format (AnimationFormatPolicy::pivotTag);
     *                 empty disables the pivot derivation.
     * @return true and sets `target` when the node is mapped, false when it is skipped.
     */
    bool tryMap(const std::string& source, std::string& target, std::string_view pivotTag = {}) const;

    /** Legacy helper: returns the mapped name, or an empty string when skipped. */
    [[nodiscard]] std::string mapBoneName(const std::string& animationBoneName) const;

    void printMappings() const;

private:
    std::unordered_map<std::string, std::string> m_RetargetMap;
    Mode m_Mode = Mode::Strict;
    bool m_Loaded = false;
};