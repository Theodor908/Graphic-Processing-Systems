#pragma once
#include "scenes/scene.hpp"

class P5Scene : public Scene {
public:
    P5Scene() : Scene("P5 - Assignment 5") {}
    void Load() override;
    void Update() override;
    void Render() override;
    void Unload() override;
};
