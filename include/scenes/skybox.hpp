#pragma once
#include "glad.h"
#include "shaders/shader.hpp"
#include <glm/glm.hpp>
#include <string>

class Skybox {
public:
    void Load();
    void Render(const glm::mat4& view, const glm::mat4& projection);
    void Unload();

private:
    Shader shader;
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    unsigned int cubemapTexture = 0;

    unsigned int LoadCubemap();
};
