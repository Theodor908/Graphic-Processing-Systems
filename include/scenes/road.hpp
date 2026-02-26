#pragma once
#include "glad.h"
#include "shaders/shader.hpp"
#include <glm/glm.hpp>
#include <string>

class Road {
public:
    void Load();
    void Render(const glm::mat4& view, const glm::mat4& projection);
    void Unload();
    void DrawGeometry();
    unsigned int GetTexture() const { return texture; }

private:
    Shader shader;
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    unsigned int texture = 0;
    int indexCount = 0;

    // Circuit parameters — tweak these to change the road shape
    int segments = 64;          // how many slices around the loop (more = smoother)
    float outerRadiusX = 40.0f; // ellipse X radius (outer edge)
    float outerRadiusZ = 30.0f; // ellipse Z radius (outer edge)
    float roadWidth = 5.0f;     // distance between inner and outer edge
    float roadY = 1.02f;        // height above terrain (avoids z-fighting)

    void GenerateGeometry();
    unsigned int LoadTexture(const std::string& path);
};
