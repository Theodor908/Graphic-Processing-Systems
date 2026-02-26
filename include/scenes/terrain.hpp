#pragma once
#include "glad.h"
#include "shaders/shader.hpp"
#include "scenes/terrain_generator.hpp"
#include <glm/glm.hpp>
#include <string>

class Terrain {
public:
    void Load();
    void Load(TerrainGenerator* generator);
    void Render(const glm::mat4& view, const glm::mat4& projection);
    void Unload();

private:
    Shader shader;
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    unsigned int texture = 0;
    int indexCount = 0;

    int gridSize = 200;

    unsigned int LoadTexture(const std::string& path);
    void GenerateMesh(TerrainGenerator* generator);
};
