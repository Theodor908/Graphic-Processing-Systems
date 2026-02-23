#pragma once
#include "scenes/scene.hpp"

class P4Scene : public Scene {
public:
    P4Scene() : Scene("P4 - Assignment 4") {}
    void Load() override;
    void Update() override;
    void Render() override;
    void Unload() override;
};
