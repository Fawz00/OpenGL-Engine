#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include "Animation.hpp"
#include "AnimationRetarget.hpp"
#include "Model.hpp"
#include "Rig.hpp"
#include "Shader.hpp"

/**
 * Animator: runtime animation state for ONE character instance.
 *
 * It only reads data: the Rig (through the Model) and an AnimationClip (through an Animation),
 * plus an optional AnimationRetarget. Everything that changes at runtime (time, keyframe
 * cursors, bone matrices, the GPU buffer) lives here, so many Animators can share one
 * Model and one Animation.
 *
 * Typical use
 * -----------
 *   Animator animator(model);                    // starts in bind pose
 *   animator.play(animation, &retarget);         // bind ONCE (names resolved, retarget applied)
 *   // every frame:
 *   animator.update();                           // CPU only, uses Time::getLastDeltaTime()
 *   animator.bind(shader);                       // uploads (if the pose changed) + binds the UBO
 *   model.draw(shader);
 *
 * Lifetime: the Animation passed to play() must outlive the Animator (or until the next
 * play()/stop()). The Rig is shared, so the Model object itself may be moved or destroyed.
 *
 * Requires OpenGL 3.3 Core only (uniform buffer objects).
 */
class Animator {
public:
    /**
     * Size of `finalBonesMatrices[]` in the shader. MUST match MAX_BONES in the vertex shader
     * (change both together). The uniform buffer holds MaxBones * 64 bytes; OpenGL 3.3 only guarantees
     * 16384 bytes per uniform block (256 matrices), and the real limit is checked when an Animator is created.
     */
    static constexpr uint32_t MaxBones = 192;
    /** Uniform buffer binding point of the `Bones` block. */
    static constexpr GLuint BonesBindingPoint = 0;
    /** Default animation tick limit in Hz. */
    static constexpr float DefaultTickRateLimit = 60.0f;

    /**
     * Which file provides the STATIC local transform of nodes that no channel animates
     * (for example the Armature / root conversion nodes above the skeleton, or bones the
     * retarget map leaves out).
     *  - Animation (default): the value stored in the animation file. This matches the original
     *    code, where the whole hierarchy came from the animation file, so root scale/rotation
     *    conversions carried by the animation are kept.
     *  - Rig: the value stored in the model file (its bind pose).
     * Nodes that exist only in one of the two files always use that file's value.
     * Only affects the next play().
     */
    enum class StaticSource { Animation, Rig };

    /** Creates an animator for a model. If the model has no rig, the animator is invalid and does nothing. */
    explicit Animator(const Model& model);
    ~Animator() = default;

    // No copy, move allowed (the GL buffer is owned by a move-only handle)
    Animator(const Animator&) = delete;
    Animator& operator=(const Animator&) = delete;
    Animator(Animator&&) noexcept = default;
    Animator& operator=(Animator&&) noexcept = default;

    /**
     * Binds a clip of an animation to the model rig and starts playing it from t = 0 (looping).
     * All the expensive work (name matching, retarget, pivot folding) happens here, once.
     * On failure the previous state is kept and false is returned (details are logged).
     *
     * @param retarget optional name map (animation -> model). nullptr = names match 1:1.
     *                 It is only read during this call and is not stored.
     * @param clipIndex clip inside the file (0 by default).
     */
    bool play(const Animation& animation, const AnimationRetarget* retarget = nullptr, std::size_t clipIndex = 0);

    /** Same as above, selecting the clip by name ("clip_<n>" when the file gave no name). */
    bool play(const Animation& animation, std::string_view clipName, const AnimationRetarget* retarget = nullptr);

    /** Stops playing and goes back to the bind pose. */
    void stop();

    /** Advances time using Time::getLastDeltaTime() (scaled) and refreshes the pose if needed. CPU only. */
    void update();
    /** Advances time by `deltaTime` seconds and refreshes the pose if needed. CPU only. */
    void update(float deltaTime);

    /**
     * Uploads the bone matrices if the pose changed since the last upload, then binds the
     * uniform buffer to BonesBindingPoint and links the shader's `Bones` block to it.
     * Call it before drawing the model with this animator's pose.
     */
    void bind(Shader& shader);

    /**
     * Animation tick limiter. The pose is evaluated at most `hz` times per second when the
     * clip's own tick rate is higher than `hz`; frames in between reuse the cached matrices
     * (no recalculation, no upload). If the clip's tick rate is lower than or equal to the
     * limit, the pose is evaluated every frame as usual. hz <= 0 disables the limiter.
     */
    void setTickRateLimit(float hz) noexcept { m_TickRateLimit = hz; }
    [[nodiscard]] float getTickRateLimit() const noexcept { return m_TickRateLimit; }

    /** Selects the source of static transforms. Takes effect on the next play(). */
    void setStaticSource(StaticSource source) noexcept { m_StaticSource = source; }
    [[nodiscard]] StaticSource getStaticSource() const noexcept { return m_StaticSource; }

    [[nodiscard]] bool isValid() const noexcept { return m_Valid; }
    [[nodiscard]] bool isPlaying() const noexcept { return m_Clip != nullptr; }
    /** Current playback time in seconds, inside [0, clip duration). */
    [[nodiscard]] double getTimeSeconds() const noexcept { return m_TimeSeconds; }
    [[nodiscard]] const std::vector<glm::mat4>& getFinalBoneMatrices() const noexcept { return m_FinalBoneMatrices; }

private:
    /** Move-only RAII owner of the uniform buffer. */
    class BoneBuffer {
    public:
        BoneBuffer() = default;
        ~BoneBuffer();
        BoneBuffer(const BoneBuffer&) = delete;
        BoneBuffer& operator=(const BoneBuffer&) = delete;
        BoneBuffer(BoneBuffer&& other) noexcept;
        BoneBuffer& operator=(BoneBuffer&& other) noexcept;

        void create(std::size_t sizeInBytes);
        [[nodiscard]] GLuint id() const noexcept { return m_Id; }

    private:
        GLuint m_Id = 0;
    };

    /** How to compute the local transform of one rig node: a range inside Binding::sources. */
    struct NodeSources {
        int32_t first = 0;
        int32_t count = 0; // 0 => use Binding::staticLocal
    };

    /** One factor of a node's local transform: either a clip channel or a constant matrix. */
    struct Source {
        int32_t channel = -1; // >= 0: evaluate this channel; -1: use `matrix`
        glm::mat4 matrix{ 1.0f };
    };

    /** Result of binding a clip to the rig. Read-only while playing, except `cursors`. */
    struct Binding {
        std::vector<NodeSources> nodes;                 // per rig node
        std::vector<Source> sources;
        std::vector<glm::mat4> staticLocal;             // per rig node (used when nodes[i].count == 0)
        std::vector<AnimationClip::Cursor> cursors;     // per clip channel
    };

    bool buildBinding(const Animation& animation, const AnimationClip& clip,
        const AnimationRetarget* retarget, Binding& out) const;
    void applyBinding(Binding&& binding, const AnimationClip* clip);
    void evaluate(float ticks);

    std::shared_ptr<const Rig> m_Rig;
    bool m_Valid = false;

    // Playback
    const AnimationClip* m_Clip = nullptr;
    Binding m_Binding;
    double m_TimeSeconds = 0.0;
    double m_DurationSeconds = 0.0;
    double m_TicksPerSecond = 0.0;
    float m_TickRateLimit = DefaultTickRateLimit;
    StaticSource m_StaticSource = StaticSource::Animation;

    // Pose cache
    std::vector<glm::mat4> m_Global;            // scratch, per rig node
    std::vector<glm::mat4> m_FinalBoneMatrices; // per bone slot
    float m_LastEvalTicks = 0.0f;
    bool m_HasPose = false;
    bool m_PoseDirty = true;                    // GPU copy is out of date

    // GPU
    BoneBuffer m_Buffer;
    GLuint m_LastProgram = 0;
};