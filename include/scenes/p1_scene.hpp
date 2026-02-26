#pragma once
#include "scenes/scene3d.hpp"
#include "shaders/shader.hpp"

class P1Scene : public Scene3D {
public:
    P1Scene() : Scene3D("Skybox", glm::vec3(0.0f, 2.0f, 3.0f)) {}

    void OnLoad() override;
    void OnUpdate() override;
    void OnRender(const glm::mat4& view, const glm::mat4& projection) override;
    void OnUnload() override;

private:
    Shader shader;
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    float rotationAngle = 0.0f;
};
