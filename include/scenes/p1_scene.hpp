#pragma once
#include "scenes/scene.hpp"
#include "shaders/shader.hpp"

class P1Scene : public Scene {
public:
    P1Scene() : Scene("Scene 1") {}

    void Load() override;
    void Update() override;
    void Render() override;
    void Unload() override;

private:
    Shader shader;
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
};
