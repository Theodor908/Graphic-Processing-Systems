#pragma once
#include "scenes/scene.hpp"
#include "scenes/skybox.hpp"
#include "shaders/shader.hpp"
#include <glm/glm.hpp>

class P1Scene : public Scene {
public:
    P1Scene() : Scene("Skybox") {}

    void Load() override;
    void Update() override;
    void Render() override;
    void Unload() override;

private:
    Skybox skybox;
    Shader shader;
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    float rotationAngle = 0.0f;

    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
     glm::vec3 cameraDir = glm::vec3(0.0f, 0.0f, -1.0f);
    float yaw = -90.0f;
    float pitch = 0.0f;
};
