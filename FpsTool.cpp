#include "FpsTool.hpp"

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

void FpsTool::resetTimer() {
    frameStartTime = clock::now();
}
