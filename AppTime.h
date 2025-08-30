#pragma once
#include <chrono>

class AppTime {
public:
    static float getTime();
    static float getCurrentDeltaTime();
    static float getLastDeltaTime();

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
};
