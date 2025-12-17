#include "Time.hpp"

void Time::update() {
    auto now = clock::now();

    if (!initialized) {
        startTime = now;
        lastFrameTime = now;
        scaledTime = 0.0f;
        initialized = true;
    }

    auto duration = std::chrono::duration<float>(now - lastFrameTime).count();
    lastDeltaTime = currentDeltaTime;
    currentDeltaTime = duration;
    lastFrameTime = now;

    scaledTime += currentDeltaTime * timeScale;
}

float Time::getApplicationTime() {
    if (!initialized) return 0.0f;
    auto now = clock::now();
    float elapsed = std::chrono::duration<float>(now - startTime).count();
    return elapsed;
}

float Time::getTime(bool scaled) {
    if (!initialized) return 0.0f;
    if (!scaled) return getApplicationTime();

    return scaledTime;
}

float Time::getCurrentDeltaTime(bool scaled) {
    return scaled ? currentDeltaTime * timeScale : currentDeltaTime;
}

float Time::getLastDeltaTime(bool scaled) {
    return scaled ? lastDeltaTime * timeScale : lastDeltaTime;
}
