#include "AppTime.h"

void AppTime::update() {
    auto now = clock::now();

    if (!initialized) {
        startTime = now;
        lastFrameTime = now;
        initialized = true;
    }

    auto duration = std::chrono::duration<float>(now - lastFrameTime).count();
    lastDeltaTime = currentDeltaTime;
    currentDeltaTime = duration;
    lastFrameTime = now;
}

float AppTime::getTime() {
    if (!initialized) return 0.0f;
    auto now = clock::now();
    return std::chrono::duration<float>(now - startTime).count();
}

float AppTime::getCurrentDeltaTime() {
    return currentDeltaTime;
}

float AppTime::getLastDeltaTime() {
    return lastDeltaTime;
}
