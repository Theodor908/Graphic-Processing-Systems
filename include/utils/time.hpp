#pragma once
#include "glfw3.h"

struct Time {
    static float deltaTime;
    static float time;
    static float lastFrameTime;

    static void Update() {
        float currentTime = (float)glfwGetTime();
        deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        time = currentTime;
    }

    static void Reset() {
        lastFrameTime = (float)glfwGetTime();
        deltaTime = 0.0f;
        time = lastFrameTime;
    }
};
