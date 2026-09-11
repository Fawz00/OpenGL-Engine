#include "AnimationRetarget.hpp"
#include "Debug.hpp"
#include <fstream>

AnimationRetarget::AnimationRetarget(const std::string& jsonPath, Mode mode)
    : m_Mode(mode)
{
    loadFromFile(jsonPath);
}

bool AnimationRetarget::loadFromFile(const std::string& jsonPath) {
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        Debug::logError("Failed to open retarget file: " + jsonPath);
        return false;
    }

    nlohmann::json jsonData;
    file >> jsonData;

    for (auto& [animBone, modelBone] : jsonData.items()) {
        m_RetargetMap[animBone] = modelBone;
    }

    Debug::log("Loaded retarget map with " + std::to_string(m_RetargetMap.size()) + " entries.");
    return true;
}

void AnimationRetarget::setMode(Mode mode) {
    m_Mode = mode;
}

AnimationRetarget::Mode AnimationRetarget::getMode() const {
    return m_Mode;
}

std::string AnimationRetarget::mapBoneName(const std::string& animationBoneName) const {
    auto it = m_RetargetMap.find(animationBoneName);
    if (it != m_RetargetMap.end()) {
        return it->second;
    }

	// Not found
    if (m_Mode == Mode::Flexible) {
		return animationBoneName; // fallback to original name
    }
    return ""; // Strict mode
}

void AnimationRetarget::printMappings() const {
    for (const auto& [src, dst] : m_RetargetMap) {
        Debug::log(src + " -> " + dst);
    }
}
