#pragma once
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

class AnimationRetarget {
public:
    enum class Mode {
        Auto,
        Flexible,
        Strict
    };

    AnimationRetarget() = default;
    explicit AnimationRetarget(const std::string& jsonPath, Mode mode = Mode::Auto);

    bool loadFromFile(const std::string& jsonPath);
    void setMode(Mode mode);
    Mode getMode() const;

    std::string mapBoneName(const std::string& animationBoneName) const;

    void printMappings() const;

private:
    std::unordered_map<std::string, std::string> m_RetargetMap;
    Mode m_Mode = Mode::Strict;
};
