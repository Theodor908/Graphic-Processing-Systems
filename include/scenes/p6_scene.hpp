#pragma once
#include "scenes/scene3d.hpp"

class P6Scene : public Scene3D {
public:
    P6Scene() : Scene3D({
        .name = "Black Hole",
        .cameraPos = glm::vec3(0.0f, 5.0f, 20.0f),
        .farPlane = 500.0f,
        .useTerrain = false,
        .useLighting = false
    }) {}

    void OnLoad() override {}
    void OnUpdate() override {}
    void OnRender(const glm::mat4& view, const glm::mat4& projection) override {}
    void OnUnload() override {}
};
