#pragma once
#include "scenes/scene.hpp"

class P4Scene : public Scene {
public:
    P4Scene() : Scene("Scene 4") {}
    void Load() override;
    void Update() override;
    void Render() override;
    void Unload() override;
};
