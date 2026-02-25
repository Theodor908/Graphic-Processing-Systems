#pragma once
#include "scenes/scene.hpp"

class P3Scene : public Scene {
public:
    P3Scene() : Scene("Scene 3") {}
    void Load() override;
    void Update() override;
    void Render() override;
    void Unload() override;
};
