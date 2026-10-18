#pragma once

#include <thread>
#include <stdexcept>
#include <chrono>
#include <unordered_map>

struct Timer {
    std::chrono::steady_clock::time_point startTime;
    bool active;
};

class FpsTool {
public:
    static void setTargetFps(int fps);
    static double getFps();
    static void endFrame();
    static void enableLimit(bool enable);

    static void startTimer(int id);
	static std::chrono::nanoseconds stopTimer(int id);

private:
    using clock = std::chrono::steady_clock;
    using time_point = std::chrono::time_point<clock>;
    using nanoseconds = std::chrono::nanoseconds;
    static std::unordered_map<int, Timer> timers;

    static void resetTimer();

    static inline nanoseconds frameInterval{};
    static inline time_point frameStartTime{};
    static inline time_point frameEndTime{};
    static inline nanoseconds frameDuration{};
    static inline nanoseconds sleepDuration{};
    static inline double lastCalculatedFps{ 0.0 };
    static inline bool limitFps{ false };
};
