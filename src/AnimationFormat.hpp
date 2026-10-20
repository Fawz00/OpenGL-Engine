#pragma once

#include <cctype>
#include <string_view>

/**
 * Source file format of an animation (and, in the future, of a model).
 *
 * Almost all of the animation code is format independent: it only looks at node names and
 * transforms. The few places where a format needs different handling are collected in
 * AnimationFormatPolicy below, so adding a format means adding one enum value, one case in
 * detectAnimationFormat() and one policy entry. No other file needs an `if (fbx)`.
 */
enum class AnimationSourceFormat {
    Auto,     // detect from the file extension (only meaningful as an argument)
    Generic,  // unknown extension: no special handling
    FBX,
    GLTF,     // .gltf / .glb
    Collada,  // .dae
};

/** Everything that differs between formats. Read by Animation and Animator. */
struct AnimationFormatPolicy {
    /**
     * Marker that the importer puts in the names of helper (pivot) nodes: "<base node><marker><kind>".
     * Retarget maps then only need the base node, and its pivots follow it.
     * Empty = the format has no such nodes.
     */
    std::string_view pivotTag;

    /** Set the importer properties that only the FBX importer understands (skip materials, cameras...). */
    bool applyFbxReaderOptions;

    /** Tick rate used when the file does not declare one. */
    float fallbackTicksPerSecond;
};

/** Detects the format from the file extension (case-insensitive). Unknown extensions give Generic. */
inline AnimationSourceFormat detectAnimationFormat(std::string_view path)
{
    const size_t slash = path.find_last_of("/\\");
    const size_t dot = path.find_last_of('.');
    if (dot == std::string_view::npos || (slash != std::string_view::npos && dot < slash)) {
        return AnimationSourceFormat::Generic;
    }

    std::string_view ext = path.substr(dot + 1);
    auto equalsIgnoreCase = [&](std::string_view lower) {
        if (ext.size() != lower.size()) return false;
        for (size_t i = 0; i < ext.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(ext[i])) != lower[i]) return false;
        }
        return true;
        };

    if (equalsIgnoreCase("fbx")) return AnimationSourceFormat::FBX;
    if (equalsIgnoreCase("gltf") || equalsIgnoreCase("glb")) return AnimationSourceFormat::GLTF;
    if (equalsIgnoreCase("dae")) return AnimationSourceFormat::Collada;
    return AnimationSourceFormat::Generic;
}

inline constexpr AnimationFormatPolicy kFbxPolicy{ "_$AssimpFbx$_", true, 25.0f };
inline constexpr AnimationFormatPolicy kGenericPolicy{ {}, false, 25.0f };

/** Returns the handling rules of a format. Auto is treated as Generic (resolve it with detectAnimationFormat first). */
inline const AnimationFormatPolicy& getAnimationFormatPolicy(AnimationSourceFormat format)
{
    switch (format) {
    case AnimationSourceFormat::FBX:
        return kFbxPolicy;

        // Add format specific rules here when a format needs them (own policy constant + own case).
    case AnimationSourceFormat::GLTF:
    case AnimationSourceFormat::Collada:
    case AnimationSourceFormat::Generic:
    case AnimationSourceFormat::Auto:
    default:
        return kGenericPolicy;
    }
}