#pragma once

#include <chrono>

class Time {
public:
    static float getApplicationTime();
    static float getTime(bool scaled = true);
    static float getCurrentDeltaTime(bool scaled = true);
    static float getLastDeltaTime(bool scaled = true);
    static inline float timeScale = 1.0f;

private:
    static void update();
	friend int main();

    using clock = std::chrono::steady_clock;
    using time_point = std::chrono::time_point<clock>;

    static inline time_point startTime{};
    static inline time_point lastFrameTime{};
    static inline float currentDeltaTime{ 0.0f };
    static inline float lastDeltaTime{ 0.0f };
    static inline bool initialized{ false };
    static inline float scaledTime{ 0.0f };
};
