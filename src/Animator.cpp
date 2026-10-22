#include "Animator.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include "Debug.hpp"
#include "Time.hpp"

namespace {
    constexpr std::size_t kBufferSize = static_cast<std::size_t>(Animator::MaxBones) * sizeof(glm::mat4);
}

// ============================================================================
// BoneBuffer
// ============================================================================

Animator::BoneBuffer::~BoneBuffer()
{
    if (m_Id != 0) glDeleteBuffers(1, &m_Id);
}

Animator::BoneBuffer::BoneBuffer(BoneBuffer&& other) noexcept
    : m_Id(std::exchange(other.m_Id, 0))
{
}

Animator::BoneBuffer& Animator::BoneBuffer::operator=(BoneBuffer&& other) noexcept
{
    if (this != &other) {
        if (m_Id != 0) glDeleteBuffers(1, &m_Id);
        m_Id = std::exchange(other.m_Id, 0);
    }
    return *this;
}

void Animator::BoneBuffer::create(std::size_t sizeInBytes)
{
    glGenBuffers(1, &m_Id);
    glBindBuffer(GL_UNIFORM_BUFFER, m_Id);
    // Allocated once at full size; per frame only the used part is updated.
    glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(sizeInBytes), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

// ============================================================================
// Animator
// ============================================================================

Animator::Animator(const Model& model)
    : m_Rig(model.getRigHandle())
{
    if (!m_Rig || m_Rig->empty()) {
        Debug::logError("ANIMATOR: the model has no rig. This animator will do nothing.");
        return;
    }

    if (static_cast<uint32_t>(m_Rig->getBoneCount()) > MaxBones) {
        Debug::logError("ANIMATOR: the rig has " + std::to_string(m_Rig->getBoneCount())
            + " bones but Animator::MaxBones / MAX_BONES in the shader is " + std::to_string(MaxBones)
            + ". Bones above the limit read as zero matrices in the shader, so vertices weighted to them"
            " collapse toward the world origin. Raise both values.");
    }

    GLint maxBlockSize = 0;
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxBlockSize);
    if (maxBlockSize > 0 && kBufferSize > static_cast<std::size_t>(maxBlockSize)) {
        Debug::logError("ANIMATOR: MaxBones (" + std::to_string(MaxBones) + ") needs " + std::to_string(kBufferSize)
            + " bytes but this GPU allows only " + std::to_string(maxBlockSize)
            + " bytes per uniform block (GL_MAX_UNIFORM_BLOCK_SIZE). Lower MaxBones or move the bones to a texture.");
    }

    m_Valid = true;
    m_Global.assign(m_Rig->getNodeCount(), glm::mat4(1.0f));
    m_FinalBoneMatrices.assign(m_Rig->getBoneCount(), glm::mat4(1.0f));
    m_Buffer.create(kBufferSize);

    // Start in bind pose: an empty binding evaluates every node with its rig bind transform.
    stop();
}

bool Animator::play(const Animation& animation, const AnimationRetarget* retarget, std::size_t clipIndex)
{
    if (!m_Valid) {
        Debug::logError("ANIMATOR: play() called on an invalid animator.");
        return false;
    }
    if (!animation.isValid()) {
        Debug::logError("ANIMATOR: play() called with an animation that failed to load.");
        return false;
    }
    if (clipIndex >= animation.getClipCount()) {
        Debug::logError("ANIMATOR: clip index " + std::to_string(clipIndex) + " is out of range ("
            + std::to_string(animation.getClipCount()) + " clip(s) in " + animation.getPath() + ").");
        return false;
    }

    const AnimationClip& clip = animation.getClip(clipIndex);

    Binding binding;
    if (!buildBinding(animation, clip, retarget, binding)) {
        return false; // previous state untouched
    }

    applyBinding(std::move(binding), &clip);
    return true;
}

bool Animator::play(const Animation& animation, std::string_view clipName, const AnimationRetarget* retarget)
{
    const int32_t index = animation.findClip(clipName);
    if (index < 0) {
        Debug::logError("ANIMATOR: clip '" + std::string(clipName) + "' not found in " + animation.getPath() + ".");
        return false;
    }
    return play(animation, retarget, static_cast<std::size_t>(index));
}

void Animator::stop()
{
    if (!m_Valid) return;

    // Every node evaluated from its bind transform.
    Binding binding;
    const int32_t nodeCount = m_Rig->getNodeCount();
    binding.nodes.assign(nodeCount, NodeSources{});
    binding.staticLocal.assign(m_Rig->getNodeBindLocals().begin(), m_Rig->getNodeBindLocals().end());
    applyBinding(std::move(binding), nullptr);
}

void Animator::applyBinding(Binding&& binding, const AnimationClip* clip)
{
    m_Binding = std::move(binding);
    m_Clip = clip;
    m_TimeSeconds = 0.0;
    m_DurationSeconds = clip ? clip->getDurationSeconds() : 0.0;
    m_TicksPerSecond = clip ? static_cast<double>(clip->getTicksPerSecond()) : 0.0;

    evaluate(0.0f);
    m_LastEvalTicks = 0.0f;
    m_HasPose = true;
    m_PoseDirty = true;
}

void Animator::update()
{
    update(Time::getLastDeltaTime());
}

void Animator::update(float deltaTime)
{
    if (!m_Valid || !m_Clip) return;

    // Advance and loop. fmod is skipped unless the clip actually wrapped; a zero-length clip
    // stays at t = 0 (the old code produced NaN with fmod(x, 0)).
    if (deltaTime > 0.0f && m_DurationSeconds > 0.0) {
        m_TimeSeconds += deltaTime;
        if (m_TimeSeconds >= m_DurationSeconds) {
            m_TimeSeconds = std::fmod(m_TimeSeconds, m_DurationSeconds);
        }
    }

    // Tick limiter: when the clip runs faster than the limit, snap time down to the
    // previous limiter step. Between two steps the evaluated time does not change, so the
    // cached pose is reused.
    double evalSeconds = m_TimeSeconds;
    if (m_TickRateLimit > 0.0f && m_TicksPerSecond > static_cast<double>(m_TickRateLimit)) {
        const double limit = static_cast<double>(m_TickRateLimit);
        evalSeconds = std::floor(m_TimeSeconds * limit) / limit;
    }

    const float ticks = static_cast<float>(evalSeconds * m_TicksPerSecond);
    if (m_HasPose && ticks == m_LastEvalTicks) {
        return; // cache hit: nothing to compute, nothing to upload
    }

    evaluate(ticks);
    m_LastEvalTicks = ticks;
    m_HasPose = true;
    m_PoseDirty = true;
}

void Animator::bind(Shader& shader)
{
    if (!m_Valid) return;

    // Binds to the indexed binding point (and to the generic target, so the update below needs no extra bind).
    glBindBufferBase(GL_UNIFORM_BUFFER, BonesBindingPoint, m_Buffer.id());

    if (m_PoseDirty) {
        const std::size_t uploadCount = std::min<std::size_t>(m_FinalBoneMatrices.size(), MaxBones);
        glBufferSubData(GL_UNIFORM_BUFFER, 0,
            static_cast<GLsizeiptr>(uploadCount * sizeof(glm::mat4)), m_FinalBoneMatrices.data());
        m_PoseDirty = false;
    }

    // Link the shader's `Bones` block to the binding point (once per program).
    // Note: the default block binding is 0, so this is also safe if a program id gets reused.
    const GLuint program = shader.getID();
    if (program != m_LastProgram) {
        const GLuint blockIndex = glGetUniformBlockIndex(program, "Bones");
        if (blockIndex != GL_INVALID_INDEX) {
            glUniformBlockBinding(program, blockIndex, BonesBindingPoint);
        }
        m_LastProgram = program;
    }
}

// ============================================================================
// HOT PATH: one linear loop over the rig nodes (pre-order, parent before child)
// ============================================================================

void Animator::evaluate(float ticks)
{
    const Rig& rig = *m_Rig;
    const int32_t nodeCount = rig.getNodeCount();

    const int32_t* parent = rig.getNodeParents().data();
    const int32_t* boneSlot = rig.getNodeBoneSlots().data();
    const glm::mat4* offsets = rig.getBoneOffsets().data();

    const NodeSources* nodes = m_Binding.nodes.data();
    const Source* sources = m_Binding.sources.data();
    const glm::mat4* staticLocal = m_Binding.staticLocal.data();
    AnimationClip::Cursor* cursors = m_Binding.cursors.data();
    const AnimationClip* clip = m_Clip;

    glm::mat4* global = m_Global.data();
    glm::mat4* bones = m_FinalBoneMatrices.data();

    for (int32_t i = 0; i < nodeCount; ++i) {
        const NodeSources& ns = nodes[i];

        glm::mat4 local;
        if (ns.count == 0) {
            local = staticLocal[i];
        }
        else {
            const Source* s = sources + ns.first;
            local = s[0].channel >= 0
                ? clip->evaluate(static_cast<uint32_t>(s[0].channel), ticks, cursors[s[0].channel])
                : s[0].matrix;

            // Rare: extra pivot nodes folded into this node (outermost first).
            for (int32_t k = 1; k < ns.count; ++k) {
                local *= s[k].channel >= 0
                    ? clip->evaluate(static_cast<uint32_t>(s[k].channel), ticks, cursors[s[k].channel])
                    : s[k].matrix;
            }
        }

        const int32_t p = parent[i];
        global[i] = p >= 0 ? global[p] * local : local;

        const int32_t slot = boneSlot[i];
        if (slot >= 0) {
            bones[slot] = global[i] * offsets[slot];
        }
    }
}

// ============================================================================
// Binding (runs once per play(), never per frame)
// ============================================================================

bool Animator::buildBinding(const Animation& animation, const AnimationClip& clip,
    const AnimationRetarget* retarget, Binding& out) const
{
    const Rig& rig = *m_Rig;
    const auto animNodes = animation.getNodes();
    const auto channels = clip.getChannels();
    const int32_t rigNodeCount = rig.getNodeCount();

    // 1) Resolve every animation node against the rig (through the retarget map, if any).
    //    mapped[a]      : the node has a target name, so its channel may drive the rig.
    //                     Always true without retarget; false for nodes a Strict map skips.
    //    counterpart[a] : rig node that REPRESENTS this animation node (looked up by its target name,
    //                     or by its own name when it is not mapped), or -1 when the rig has no such node.
    //                     A node with a counterpart is already part of the rig hierarchy, so it must never
    //                     be folded into another node (that would apply its transform twice).
    //    animOfRig[r]   : mapped animation node that DRIVES rig node r (first one wins), or -1.
    std::vector<uint8_t> mapped(animNodes.size(), 0);
    std::vector<int32_t> counterpart(animNodes.size(), -1);
    std::vector<int32_t> animOfRig(rigNodeCount, -1);
    uint32_t collisions = 0;

    // Pivot node convention of the animation's format (empty for formats without one, so the
    // derivation is simply not done for them).
    const std::string_view pivotTag = getAnimationFormatPolicy(animation.getFormat()).pivotTag;

    std::string mappedName;
    for (size_t a = 0; a < animNodes.size(); ++a) {
        const std::string* target = &animNodes[a].name;
        bool isMapped = true;
        if (retarget) {
            isMapped = retarget->tryMap(animNodes[a].name, mappedName, pivotTag);
            if (isMapped) target = &mappedName;
        }
        mapped[a] = isMapped ? 1 : 0;

        const int32_t r = rig.findNode(*target);
        counterpart[a] = r;
        if (!isMapped || r < 0) continue;

        if (animOfRig[r] >= 0) { ++collisions; continue; } // two nodes mapped to the same rig node: first wins
        animOfRig[r] = static_cast<int32_t>(a);
    }

    // Animation node that represents each rig node: its driver if there is one, otherwise a node that
    // is only matched by name (not driven). Its static transform is used according to m_StaticSource.
    std::vector<int32_t> repOfRig = animOfRig;
    for (size_t a = 0; a < animNodes.size(); ++a) {
        const int32_t r = counterpart[a];
        if (r >= 0 && repOfRig[r] < 0) repOfRig[r] = static_cast<int32_t>(a);
    }

    // 2) Channel lookup per animation node.
    std::vector<int32_t> channelOfAnim(animNodes.size(), -1);
    for (size_t c = 0; c < channels.size(); ++c) {
        const int32_t n = channels[c].nodeIndex;
        if (n >= 0 && channelOfAnim[n] < 0) channelOfAnim[n] = static_cast<int32_t>(c);
    }
    std::vector<uint8_t> channelUsed(channels.size(), 0);

    // A piece of a node's local transform, before merging constants.
    struct Piece { int32_t channel; glm::mat4 matrix; };
    AnimationClip::Cursor scratchCursor;

    auto makePiece = [&](int32_t animNode, const glm::mat4& staticMatrix) -> Piece {
        const int32_t c = mapped[animNode] ? channelOfAnim[animNode] : -1;
        if (c < 0) return { -1, staticMatrix };

        channelUsed[c] = 1;
        if (channels[c].constant) {
            // Never changes: evaluate once now and treat it as a constant.
            return { -1, clip.evaluate(static_cast<uint32_t>(c), 0.0f, scratchCursor) };
        }
        return { c, glm::mat4(1.0f) };
        };

    out.nodes.assign(rigNodeCount, NodeSources{});
    out.staticLocal.assign(rigNodeCount, glm::mat4(1.0f));
    out.sources.clear();
    out.cursors.assign(channels.size(), AnimationClip::Cursor{});

    uint32_t drivenNodes = 0;
    uint32_t discardedChains = 0;
    std::vector<Piece> pieces;
    std::vector<int32_t> extras;

    // 3) Per rig node: build the list of factors of its local transform.
    // Compares the static transform stored in the model with the one stored in the animation.
    uint32_t staticDiffCount = 0;
    std::string staticDiffText;
    auto compareStatic = [&](int32_t r, int32_t rep) {
        const glm::mat4& m1 = rig.getNodeBindLocal(r);
        const glm::mat4& m2 = animNodes[rep].local;
        glm::vec3 s1(glm::length(glm::vec3(m1[0])), glm::length(glm::vec3(m1[1])), glm::length(glm::vec3(m1[2])));
        glm::vec3 s2(glm::length(glm::vec3(m2[0])), glm::length(glm::vec3(m2[1])), glm::length(glm::vec3(m2[2])));
        if (s1.x < 1e-8f || s1.y < 1e-8f || s1.z < 1e-8f || s2.x < 1e-8f || s2.y < 1e-8f || s2.z < 1e-8f) return;
        const glm::mat3 r1(glm::vec3(m1[0]) / s1.x, glm::vec3(m1[1]) / s1.y, glm::vec3(m1[2]) / s1.z);
        const glm::mat3 r2(glm::vec3(m2[0]) / s2.x, glm::vec3(m2[1]) / s2.y, glm::vec3(m2[2]) / s2.z);
        const glm::mat3 rel = glm::transpose(r1) * r2;
        const float trace = rel[0][0] + rel[1][1] + rel[2][2];
        const float angle = glm::degrees(std::acos(glm::clamp((trace - 1.0f) * 0.5f, -1.0f, 1.0f)));
        const glm::vec3 ratio = s2 / s1;
        const float scaleRatio = (ratio.x + ratio.y + ratio.z) / 3.0f;
        const float moved = glm::length(glm::vec3(m2[3]) - glm::vec3(m1[3]));
        const bool scaleDiffers = std::abs(scaleRatio - 1.0f) > 0.01f;
        const bool moveDiffers = moved > 1e-3f * std::max(1.0f, glm::length(glm::vec3(m1[3])));
        if (angle < 0.5f && !scaleDiffers && !moveDiffers) return;

        if (staticDiffCount++ < 3) {
            if (!staticDiffText.empty()) staticDiffText += "; ";
            staticDiffText += rig.getNodeName(r) + ": rotation " + std::to_string(angle).substr(0, 5) + " deg, scale x"
                + std::to_string(scaleRatio).substr(0, 6) + ", move " + std::to_string(moved).substr(0, 6);
        }
        };

    // A local transform is only meaningful relative to ITS parent. The animation file's static value may
    // therefore be used under the model's parent only if both parents are the same node (a driven node may
    // also have pivot nodes between them, which are folded in below).
    uint32_t parentMismatchCount = 0;
    std::string parentMismatchText;
    auto parentsAgree = [&](int32_t r, int32_t rep, bool throughExtras) {
        const int32_t rigParent = rig.getNodeParent(r);
        int32_t p = animNodes[rep].parent;
        if (throughExtras) {
            while (p >= 0 && counterpart[p] < 0) p = animNodes[p].parent;
        }
        if (rigParent < 0 || p < 0) return rigParent < 0 && p < 0;
        return counterpart[p] == rigParent;
        };

    for (int32_t r = 0; r < rigNodeCount; ++r) {
        const int32_t a = animOfRig[r];
        const int32_t rep = repOfRig[r];
        const bool staticIsUsed = rep >= 0 && (channelOfAnim[rep] < 0 || !mapped[rep]);

        // Static transform of this node: from the animation file or from the model, see StaticSource.
        bool useAnimationValue = (m_StaticSource == StaticSource::Animation && rep >= 0);
        if (useAnimationValue && staticIsUsed && !parentsAgree(r, rep, a >= 0)) {
            useAnimationValue = false;
            if (parentMismatchCount++ < 3) {
                if (!parentMismatchText.empty()) parentMismatchText += ", ";
                parentMismatchText += rig.getNodeName(r);
            }
        }
        const glm::mat4& staticMatrix = useAnimationValue ? animNodes[rep].local : rig.getNodeBindLocal(r);
        if (staticIsUsed) compareStatic(r, rep);

        if (a < 0) {
            out.staticLocal[r] = staticMatrix;
            continue;
        }
        ++drivenNodes;

        // Nodes between this animation node and its nearest ancestor that the rig already contains,
        // but that do NOT exist in the rig themselves (e.g. pivot nodes of a model file exported
        // without pivots), are folded into this node so their transform is not lost.
        // Ancestors that the rig does contain (even if the retarget map does not drive them) are
        // anchors: the rig applies them, so folding them too would double their transform.
        // If no anchor exists above, the chain is dropped.
        extras.clear();
        bool anchored = false;
        for (int32_t p = animNodes[a].parent; p >= 0; p = animNodes[p].parent) {
            if (counterpart[p] >= 0) { anchored = true; break; }
            extras.push_back(p);
        }
        if (!anchored && !extras.empty()) {
            ++discardedChains;
            extras.clear();
        }

        pieces.clear();
        for (auto it = extras.rbegin(); it != extras.rend(); ++it) { // outermost first
            pieces.push_back(makePiece(*it, animNodes[*it].local));
        }
        // The node itself: its channel, otherwise its static transform.
        pieces.push_back(makePiece(a, staticMatrix));

        // Merge neighbouring constants so the hot loop multiplies as little as possible.
        std::vector<Piece> merged;
        for (const Piece& piece : pieces) {
            if (piece.channel < 0 && !merged.empty() && merged.back().channel < 0) {
                merged.back().matrix = merged.back().matrix * piece.matrix;
            }
            else {
                merged.push_back(piece);
            }
        }

        if (merged.size() == 1 && merged[0].channel < 0) {
            out.staticLocal[r] = merged[0].matrix;
            continue;
        }

        out.nodes[r].first = static_cast<int32_t>(out.sources.size());
        out.nodes[r].count = static_cast<int32_t>(merged.size());
        for (const Piece& piece : merged) {
            out.sources.push_back({ piece.channel, piece.matrix });
        }
    }

    // 4) Report (once).
    std::string unusedNames;
    uint32_t unused = 0;
    for (size_t c = 0; c < channels.size(); ++c) {
        if (channelUsed[c]) continue;
        if (unused++ < 5) {
            if (!unusedNames.empty()) unusedNames += ", ";
            unusedNames += channels[c].nodeName;
        }
    }

    // Bones (nodes that carry skin weights) that no animation node drives stay in bind pose.
    // Vertices weighted between them and animated bones get stretched, so list them.
    std::string undrivenNames;
    uint32_t undrivenBones = 0;
    for (int32_t r = 0; r < rigNodeCount; ++r) {
        if (rig.getNodeBoneSlot(r) < 0 || animOfRig[r] >= 0) continue;
        if (undrivenBones++ < 5) {
            if (!undrivenNames.empty()) undrivenNames += ", ";
            undrivenNames += rig.getNodeName(r);
        }
    }

    if (drivenNodes == 0) {
        Debug::logError("ANIMATOR: no animation node of clip '" + clip.getName() + "' matches a node of the model rig."
            " Check the node names or the retarget map.");
        return false;
    }

    // The logger keeps only a short fixed-size message (Debug::LOG_MESSAGE_SIZE), so the report is split into one
    // short summary line plus one detail line per topic. Every line is capped, so nothing is cut off silently.
    auto capped = [](std::string text) {
        constexpr size_t kMaxLineLength = 380;
        if (text.size() > kMaxLineLength) text = text.substr(0, kMaxLineLength) + "...";
        return text;
        };

    const bool warn = unused > 0 || undrivenBones > 0 || staticDiffCount > 0 || parentMismatchCount > 0
        || collisions > 0 || discardedChains > 0;

    std::string summary = "ANIMATOR: bound clip '" + clip.getName() + "': " + std::to_string(drivenNodes) + "/"
        + std::to_string(rigNodeCount) + " rig nodes animated";
    if (unused > 0) summary += ", " + std::to_string(unused) + " unused channel(s)";
    if (undrivenBones > 0) summary += ", " + std::to_string(undrivenBones) + " bone(s) not driven";
    if (staticDiffCount > 0) summary += ", " + std::to_string(staticDiffCount) + " static transform difference(s)";
    if (parentMismatchCount > 0) summary += ", " + std::to_string(parentMismatchCount) + " parent mismatch(es)";
    if (collisions > 0) summary += ", " + std::to_string(collisions) + " mapping collision(s)";
    if (discardedChains > 0) summary += ", " + std::to_string(discardedChains) + " pivot chain(s) dropped";
    if (warn) Debug::logWarn(capped(summary));
    else Debug::log(capped(summary));

    if (unused > 0) {
        Debug::logWarn(capped("ANIMATOR:   unused channels (their node is not driven, so the channel is ignored), first: " + unusedNames));
    }
    if (undrivenBones > 0) {
        Debug::logWarn(capped("ANIMATOR:   bones not driven by any animation node, they stay in bind pose, first: " + undrivenNames));
    }
    if (staticDiffCount > 0) {
        Debug::logWarn(capped("ANIMATOR:   non-animated nodes whose static transform differs between model and animation (using the "
            + std::string(m_StaticSource == StaticSource::Animation ? "animation's" : "model's") + " value), first: " + staticDiffText));
    }
    if (parentMismatchCount > 0) {
        Debug::logWarn(capped("ANIMATOR:   nodes with a different parent in the animation file than in the model (the model's value is used), first: "
            + parentMismatchText));
    }

    return true;
}