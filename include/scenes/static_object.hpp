#pragma once
#include "glad.h"
#include "shaders/shader.hpp"
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct ObjectInstance {
    glm::vec3 position;
    glm::vec3 scale;
    unsigned int textureID;
};

class StaticObjectRenderer {
public:
    void Load();
    void Render(const std::vector<ObjectInstance>& objects,
                const glm::mat4& view, const glm::mat4& projection);
    void Unload();
    void BindAndDraw();

    // Load a texture and return its GL ID — call this to prepare textures
    unsigned int LoadTexture(const std::string& path);

private:
    Shader shader;
    unsigned int cubeVAO = 0, cubeVBO = 0, cubeEBO = 0;

    void CreateCubeMesh();
};
