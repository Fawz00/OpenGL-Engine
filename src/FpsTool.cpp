#include "FpsTool.hpp"

#include "Debug.hpp"

std::unordered_map<int, Timer> FpsTool::timers;

void FpsTool::setTargetFps(int fps) {
    if (fps <= 0) {
        throw std::invalid_argument("FPS must be greater than 0");
    }
    frameInterval = nanoseconds(1'000'000'000ll / fps);
    resetTimer();
}

double FpsTool::getFps() {
    if (frameDuration.count() > 0) {
        lastCalculatedFps = 1'000'000'000.0 /
            static_cast<double>(frameDuration.count() + sleepDuration.count());
    }
    return lastCalculatedFps;
}

void FpsTool::endFrame() {
    frameEndTime = clock::now();
    frameDuration = frameEndTime - frameStartTime;

    if (limitFps && frameDuration < frameInterval) {
        sleepDuration = frameInterval - frameDuration;
        std::this_thread::sleep_for(sleepDuration);
    }
    else {
        sleepDuration = nanoseconds{ 0 };
    }

    resetTimer();
}

void FpsTool::enableLimit(bool enable) {
    limitFps = enable;
}

void FpsTool::startTimer(int id)
{
    auto& timer = timers[id];
    if (timer.active) {
        Debug::logWarn("Timer with ID " + std::to_string(id) + " is already running.");
    }
    timer.startTime = clock::now();
	timer.active = true;
}

std::chrono::nanoseconds FpsTool::stopTimer(int id)
{
    auto it = timers.find(id);
    if (it == timers.end() || !it->second.active) {
        Debug::logWarn("Timer with ID " + std::to_string(id) + " is not running.");
        return nanoseconds{ 0 };
    }
    auto& timer = it->second;
    auto endTime = clock::now();
    auto duration = std::chrono::duration_cast<nanoseconds>(endTime - timer.startTime);
    timer.active = false;
    return duration;
}

void FpsTool::resetTimer() {
    frameStartTime = clock::now();
}
