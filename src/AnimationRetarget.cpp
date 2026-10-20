#include "AnimationRetarget.hpp"

#include <fstream>

#include "Debug.hpp"

AnimationRetarget::AnimationRetarget(const std::string& jsonPath, Mode mode)
    : m_Mode(mode)
{
    loadFromFile(jsonPath);
}

bool AnimationRetarget::loadFromFile(const std::string& jsonPath)
{
    m_RetargetMap.clear();
    m_Loaded = false;

    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        Debug::logError("Failed to open retarget file: " + jsonPath);
        return false;
    }

    // Malformed JSON throws; a bad file must not crash the engine.
    nlohmann::json jsonData = nlohmann::json::parse(file, nullptr, false);
    if (jsonData.is_discarded() || !jsonData.is_object()) {
        Debug::logError("Retarget file is not a valid JSON object: " + jsonPath);
        return false;
    }

    m_RetargetMap.reserve(jsonData.size());
    for (auto it = jsonData.begin(); it != jsonData.end(); ++it) {
        if (it.value().is_string()) {
            m_RetargetMap[it.key()] = it.value().get<std::string>();
        }
        else {
            Debug::logWarn("Retarget: ignoring non-string target for '" + it.key() + "' in " + jsonPath);
        }
    }

    m_Loaded = true;
    Debug::log("Loaded retarget map with " + std::to_string(m_RetargetMap.size()) + " entries.");
    return true;
}

void AnimationRetarget::setMode(Mode mode)
{
    m_Mode = mode;
}

AnimationRetarget::Mode AnimationRetarget::getMode() const
{
    return m_Mode;
}

bool AnimationRetarget::tryMap(const std::string& source, std::string& target, std::string_view pivotTag) const
{
    // 1) Explicit entry (always wins)
    auto it = m_RetargetMap.find(source);
    if (it != m_RetargetMap.end()) {
        if (it->second.empty()) return false; // explicit skip
        target = it->second;
        return true;
    }

    // 2) Pivot (helper) node of the format: follow the mapping of its base node
    if (!pivotTag.empty()) {
        const size_t tagPos = source.find(pivotTag);
        if (tagPos != std::string::npos && tagPos > 0) {
            auto base = m_RetargetMap.find(source.substr(0, tagPos));
            if (base != m_RetargetMap.end()) {
                if (base->second.empty()) return false; // base explicitly skipped
                target = base->second + source.substr(tagPos);
                return true;
            }
        }
    }

    // 3) Not mapped
    if (m_Mode == Mode::Flexible) {
        target = source; // fallback to original name
        return true;
    }
    return false; // Strict
}

std::string AnimationRetarget::mapBoneName(const std::string& animationBoneName) const
{
    std::string target;
    return tryMap(animationBoneName, target) ? target : std::string();
}

void AnimationRetarget::printMappings() const
{
    for (const auto& [src, dst] : m_RetargetMap) {
        Debug::log(src + " -> " + dst);
    }
}