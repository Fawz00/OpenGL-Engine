#pragma once

#include <algorithm>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


#include "AnimationFormat.hpp"

/**
 * A node of the hierarchy stored in the animation file.
 * Only nodes that are targeted by a channel, or are ancestors of such nodes, are kept.
 * `local` is the static local transform found in the file.
 *
 * The Animator uses these nodes to fold intermediate nodes that do not exist in the
 * model Rig (e.g. FBX `$AssimpFbx$` pivot nodes) into the nearest Rig node.
 */
struct AnimationNode {
    std::string name;
    int32_t parent = -1; // index in Animation::getNodes(), -1 for root
    glm::mat4 local{ 1.0f };
};

/** Range inside one of the flat keyframe arrays of a clip. */
struct TrackRange {
    uint32_t offset = 0;
    uint32_t count = 0;
};

/** Animation of one node: three tracks (position, rotation, scale). Read-only. */
struct AnimationChannel {
    std::string nodeName;
    int32_t nodeIndex = -1; // index in Animation::getNodes(), -1 if the node was not found in the file
    TrackRange position;
    TrackRange rotation;
    TrackRange scale;
    bool constant = false;  // every track has at most one key: the result never changes over time
};

/**
 * One animation timeline (clip). Read-only after loading, so any number of Animators
 * can share it. Keyframes are stored in flat arrays (struct of arrays) for cache locality.
 *
 * Runtime state (keyframe cursors, current time) lives in the Animator, never here.
 */
class AnimationClip {
public:
    /** Last keyframe index used per track. Owned by the caller (Animator), one per channel. */
    struct Cursor {
        uint32_t position = 0;
        uint32_t rotation = 0;
        uint32_t scale = 0;
    };

    [[nodiscard]] const std::string& getName() const noexcept { return m_Name; }
    /** Duration in ticks. */
    [[nodiscard]] float getDuration() const noexcept { return m_Duration; }
    [[nodiscard]] float getTicksPerSecond() const noexcept { return m_TicksPerSecond; }
    [[nodiscard]] double getDurationSeconds() const noexcept
    {
        return m_TicksPerSecond > 0.0f ? static_cast<double>(m_Duration) / m_TicksPerSecond : 0.0;
    }
    [[nodiscard]] std::span<const AnimationChannel> getChannels() const noexcept { return m_Channels; }

    /**
     * Evaluates the local transform (T * R * S) of a channel at `ticks`.
     * HOT PATH. Interpolation is clamped (no extrapolation outside the first/last key).
     * The cursor makes the keyframe search O(1) while time moves forward; it falls back
     * to a binary search when time jumps (loop wrap, seek).
     */
    [[nodiscard]] inline glm::mat4 evaluate(uint32_t channelIndex, float ticks, Cursor& cursor) const noexcept;

private:
    friend class Animation;

    static inline uint32_t seek(const float* times, uint32_t count, float t, uint32_t hint) noexcept;
    static inline float blendFactor(float t0, float t1, float t) noexcept;
    static inline glm::mat4 composeTRS(const glm::vec3& t, const glm::quat& q, const glm::vec3& s) noexcept;

    std::string m_Name;
    float m_Duration = 0.0f;
    float m_TicksPerSecond = 0.0f;

    std::vector<AnimationChannel> m_Channels;

    // Flat keyframe storage (channels reference ranges inside these arrays)
    std::vector<float> m_PositionTimes;
    std::vector<glm::vec3> m_PositionValues;
    std::vector<float> m_RotationTimes;
    std::vector<glm::quat> m_RotationValues;
    std::vector<float> m_ScaleTimes;
    std::vector<glm::vec3> m_ScaleValues;
};

/**
 * Animation: everything loaded from one animation file. Read-only.
 *
 * A file can contain several clips. Each clip is named after the file's animation name;
 * when the name is empty it becomes "clip_<index>". Names are not forced to be unique:
 * findClip() returns the first match.
 *
 * Does not depend on Model or AnimationRetarget: binding to a skeleton (and name retargeting)
 * is done once by Animator::play().
 */
class Animation {
public:
    Animation() = default;
    /**
     * Loads all clips of a file. Check isValid() afterwards (errors are logged).
     * @param format how to treat the file. Auto (default) detects it from the extension;
     *               pass a value explicitly to override it (see AnimationFormat.hpp).
     */
    explicit Animation(const std::string& animationPath, AnimationSourceFormat format = AnimationSourceFormat::Auto);
    ~Animation() = default;

    // No copy, only move
    Animation(const Animation&) = delete;
    Animation& operator=(const Animation&) = delete;
    Animation(Animation&&) noexcept = default;
    Animation& operator=(Animation&&) noexcept = default;

    [[nodiscard]] bool isValid() const noexcept { return !m_Clips.empty(); }
    [[nodiscard]] const std::string& getPath() const noexcept { return m_Path; }
    /** Format the file was treated as (never Auto). */
    [[nodiscard]] AnimationSourceFormat getFormat() const noexcept { return m_Format; }

    [[nodiscard]] size_t getClipCount() const noexcept { return m_Clips.size(); }
    [[nodiscard]] const AnimationClip& getClip(size_t index) const { return m_Clips[index]; }
    /** Index of the first clip with this name, or -1. */
    [[nodiscard]] int32_t findClip(std::string_view name) const;

    /** Hierarchy of the file (pruned, pre-order). Used when binding to a Rig. */
    [[nodiscard]] std::span<const AnimationNode> getNodes() const noexcept { return m_Nodes; }

private:
    std::string m_Path;
    AnimationSourceFormat m_Format = AnimationSourceFormat::Generic;
    std::vector<AnimationNode> m_Nodes;
    std::vector<AnimationClip> m_Clips;
};

// ============================================================================
// Hot path (inline)
// ============================================================================

inline uint32_t AnimationClip::seek(const float* times, uint32_t count, float t, uint32_t hint) noexcept
{
    // Precondition: count >= 2. Returns i in [0, count-2] with times[i] <= t < times[i+1] (clamped).
    const uint32_t last = count - 2;
    uint32_t i = hint < last ? hint : last;

    if (t >= times[i]) {
        if (t < times[i + 1]) return i;                       // same segment as last frame
        if (i < last && t < times[i + 2]) return i + 1;       // next segment (usual case)
    }

    const float* it = std::upper_bound(times, times + count, t);
    const uint32_t pos = static_cast<uint32_t>(it - times);
    i = pos == 0 ? 0 : pos - 1;
    return i < last ? i : last;
}

inline float AnimationClip::blendFactor(float t0, float t1, float t) noexcept
{
    const float length = t1 - t0;
    if (length < 1e-6f) return 0.0f;
    const float f = (t - t0) / length;
    return f < 0.0f ? 0.0f : (f > 1.0f ? 1.0f : f);
}

inline glm::mat4 AnimationClip::composeTRS(const glm::vec3& t, const glm::quat& q, const glm::vec3& s) noexcept
{
    // M = T * R * S built directly (columns of R scaled by S), no intermediate matrices.
    const glm::mat3 r = glm::mat3_cast(q);
    glm::mat4 m;
    m[0] = glm::vec4(r[0] * s.x, 0.0f);
    m[1] = glm::vec4(r[1] * s.y, 0.0f);
    m[2] = glm::vec4(r[2] * s.z, 0.0f);
    m[3] = glm::vec4(t, 1.0f);
    return m;
}

inline glm::mat4 AnimationClip::evaluate(uint32_t channelIndex, float ticks, Cursor& cursor) const noexcept
{
    const AnimationChannel& ch = m_Channels[channelIndex];

    glm::vec3 position(0.0f);
    if (ch.position.count == 1) {
        position = m_PositionValues[ch.position.offset];
    }
    else if (ch.position.count > 1) {
        const float* times = m_PositionTimes.data() + ch.position.offset;
        const glm::vec3* values = m_PositionValues.data() + ch.position.offset;
        const uint32_t i = seek(times, ch.position.count, ticks, cursor.position);
        cursor.position = i;
        position = glm::mix(values[i], values[i + 1], blendFactor(times[i], times[i + 1], ticks));
    }

    glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
    if (ch.rotation.count == 1) {
        rotation = m_RotationValues[ch.rotation.offset];
    }
    else if (ch.rotation.count > 1) {
        const float* times = m_RotationTimes.data() + ch.rotation.offset;
        const glm::quat* values = m_RotationValues.data() + ch.rotation.offset;
        const uint32_t i = seek(times, ch.rotation.count, ticks, cursor.rotation);
        cursor.rotation = i;
        rotation = glm::normalize(glm::slerp(values[i], values[i + 1], blendFactor(times[i], times[i + 1], ticks)));
    }

    glm::vec3 scale(1.0f);
    if (ch.scale.count == 1) {
        scale = m_ScaleValues[ch.scale.offset];
    }
    else if (ch.scale.count > 1) {
        const float* times = m_ScaleTimes.data() + ch.scale.offset;
        const glm::vec3* values = m_ScaleValues.data() + ch.scale.offset;
        const uint32_t i = seek(times, ch.scale.count, ticks, cursor.scale);
        cursor.scale = i;
        scale = glm::mix(values[i], values[i + 1], blendFactor(times[i], times[i + 1], ticks));
    }

    return composeTRS(position, rotation, scale);
}