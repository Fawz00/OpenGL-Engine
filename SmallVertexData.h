#pragma once

#include <cstdint>
#include <cstdlib>
#include <glm/glm.hpp>
#include <memory>
#include <cstring>
#include <cassert>

class SmallVertexData {
private:
    uint8_t flags; // bitmap komponen
    std::unique_ptr<uint8_t[], void(*)(void*)> data{nullptr, deleter}; // buffer aligned
    size_t dataSize;

    static void deleter(void* ptr) {
#if defined(_MSC_VER)
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }

    // Hitung total size dari pointer yang diberikan
    static size_t calculateDataSize(
        const glm::vec3* position,
        const glm::vec3* normal,
        const glm::vec3* tangent,
        const glm::vec3* bitangent,
        const glm::vec2* texcoord,
        const uint8_t* m_BoneIDs,
        const uint8_t* m_Weights
    ) {
        size_t size = 0;
        if (position) size += sizeof(glm::vec3);
        if (normal) size += sizeof(glm::vec3);
        if (texcoord) size += sizeof(glm::vec2);
        if (tangent) size += sizeof(glm::vec3);
        if (bitangent) size += sizeof(glm::vec3);
        if (m_BoneIDs) size += 4;
        if (m_Weights) size += 4;
        return size;
    }

public:
    // Constructor super friendly
    SmallVertexData(
        const glm::vec3* position = nullptr,
        const glm::vec3* normal = nullptr,
        const glm::vec2* texcoord = nullptr,
        const glm::vec3* tangent = nullptr,
        const glm::vec3* bitangent = nullptr,
        const uint8_t* m_BoneIDs = nullptr,
        const uint8_t* m_Weights = nullptr
    ) {
        flags = 0;
        if (position)  flags |= 0b00000001;
        if (normal)    flags |= 0b00000010;
        if (texcoord)  flags |= 0b00000100;
        if (tangent)   flags |= 0b00001000;
        if (bitangent) flags |= 0b00010000;
        if (m_BoneIDs) flags |= 0b00100000;
        if (m_Weights) flags |= 0b01000000;

        dataSize = calculateDataSize(position, normal, tangent, bitangent, texcoord, m_BoneIDs, m_Weights);

        if (dataSize > 0) {
            void* raw = nullptr;
#if defined(_MSC_VER)
            raw = _aligned_malloc(dataSize, 16);
#else
            if (posix_memalign(&raw, 16, dataSize) != 0) raw = nullptr;
#endif
            data = std::unique_ptr<uint8_t[], void(*)(void*)>(reinterpret_cast<uint8_t*>(raw), deleter);

            // Copy semua pointer ke buffer kontigu sesuai urutan
            size_t offset = 0;
            if (position) { std::memcpy(data.get() + offset, position, sizeof(glm::vec3)); offset += sizeof(glm::vec3); }
            if (normal) { std::memcpy(data.get() + offset, normal, sizeof(glm::vec3)); offset += sizeof(glm::vec3); }
            if (tangent) { std::memcpy(data.get() + offset, tangent, sizeof(glm::vec3)); offset += sizeof(glm::vec3); }
            if (bitangent) { std::memcpy(data.get() + offset, bitangent, sizeof(glm::vec3)); offset += sizeof(glm::vec3); }
            if (texcoord) { std::memcpy(data.get() + offset, texcoord, sizeof(glm::vec2)); offset += sizeof(glm::vec2); }
            if (m_BoneIDs) { std::memcpy(data.get() + offset, m_BoneIDs, 4); offset += 4; }
            if (m_Weights) { std::memcpy(data.get() + offset, m_Weights, 4); offset += 4; }
        }
    }

    // Non-copyable
    SmallVertexData(const SmallVertexData&) = delete;
    SmallVertexData& operator=(const SmallVertexData&) = delete;

    // Getter
    const glm::vec3* getPosition() const {
        if (!(flags & 0b00000001)) return nullptr;
        return reinterpret_cast<const glm::vec3*>(data.get());
    }

    const glm::vec3* getNormal() const {
        if (!(flags & 0b00000010)) return nullptr;
        size_t offset = (flags & 0b00000001) ? sizeof(glm::vec3) : 0;
        return reinterpret_cast<const glm::vec3*>(data.get() + offset);
    }

    const glm::vec2* getTexcoord() const {
        if (!(flags & 0b00000100)) return nullptr;
        size_t offset = 0;
        if (flags & 0b00000001) offset += sizeof(glm::vec3);
        if (flags & 0b00000010) offset += sizeof(glm::vec3);
        return reinterpret_cast<const glm::vec2*>(data.get() + offset);
    }

    const glm::vec3* getTangent() const {
        if (!(flags & 0b00001000)) return nullptr;
        size_t offset = 0;
        if (flags & 0b00000001) offset += sizeof(glm::vec3);
        if (flags & 0b00000010) offset += sizeof(glm::vec3);
        if (flags & 0b00000100) offset += sizeof(glm::vec2);
        return reinterpret_cast<const glm::vec3*>(data.get() + offset);
    }

    const glm::vec3* getBitangent() const {
        if (!(flags & 0b00010000)) return nullptr;
        size_t offset = 0;
        if (flags & 0b00000001) offset += sizeof(glm::vec3);
        if (flags & 0b00000010) offset += sizeof(glm::vec3);
        if (flags & 0b00000100) offset += sizeof(glm::vec2);
        if (flags & 0b00001000) offset += sizeof(glm::vec3);
        return reinterpret_cast<const glm::vec3*>(data.get() + offset);
    }

    const uint8_t* getBoneIDs() const {
        if (!(flags & 0b00100000)) return nullptr;
        size_t offset = 0;
        if (flags & 0b00000001) offset += sizeof(glm::vec3);
        if (flags & 0b00000010) offset += sizeof(glm::vec3);
        if (flags & 0b00000100) offset += sizeof(glm::vec2);
        if (flags & 0b00001000) offset += sizeof(glm::vec3);
        if (flags & 0b00010000) offset += sizeof(glm::vec3);
        return data.get() + offset;
    }

    const uint8_t* getBoneWeights() const {
        if (!(flags & 0b01000000)) return nullptr;
        size_t offset = 0;
        if (flags & 0b00000001) offset += sizeof(glm::vec3);
        if (flags & 0b00000010) offset += sizeof(glm::vec3);
        if (flags & 0b00000100) offset += sizeof(glm::vec2);
        if (flags & 0b00001000) offset += sizeof(glm::vec3);
        if (flags & 0b00010000) offset += sizeof(glm::vec3);
        if (flags & 0b00100000) offset += 4;
        return data.get() + offset;
    }

    uint8_t getFlags() const { return flags; }
    size_t getDataSize() const { return dataSize; }
};
