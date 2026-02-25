#pragma once
#include "scenes/scene.hpp"

class P2Scene : public Scene {
public:
    P2Scene() : Scene("Scene 2") {}
    void Load() override;
    void Update() override;
    void Render() override;
    void Unload() override;
};
