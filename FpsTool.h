#pragma once

#include <chrono>
#include <thread>
#include <stdexcept>

class FpsTool {
public:
    static void setTargetFps(int fps);
    static double getFps();
    static void endFrame();
    static void enableLimit(bool enable);

private:
    using clock = std::chrono::steady_clock;
    using time_point = std::chrono::time_point<clock>;
    using nanoseconds = std::chrono::nanoseconds;

    static void resetTimer();

    static inline nanoseconds frameInterval{};
    static inline time_point frameStartTime{};
    static inline time_point frameEndTime{};
    static inline nanoseconds frameDuration{};
    static inline nanoseconds sleepDuration{};
    static inline double lastCalculatedFps{ 0.0 };
    static inline bool limitFps{ true };
};
