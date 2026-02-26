#pragma once
#include "glad.h"
#include "shaders/shader.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

struct ObjectInstance {
    glm::vec3 position;
    glm::vec3 scale;
    unsigned int textureID;
    glm::vec3 rotation = glm::vec3(0.0f); // (pitch, yaw, roll) in radians
};

// Build model matrix: translate -> rotY -> rotX -> rotZ -> scale -> shift up
inline glm::mat4 ModelMatrixFromObject(const ObjectInstance& obj) {
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, obj.position);
    if (obj.rotation.y != 0.0f) m = glm::rotate(m, obj.rotation.y, glm::vec3(0,1,0));
    if (obj.rotation.x != 0.0f) m = glm::rotate(m, obj.rotation.x, glm::vec3(1,0,0));
    if (obj.rotation.z != 0.0f) m = glm::rotate(m, obj.rotation.z, glm::vec3(0,0,1));
    m = glm::scale(m, obj.scale);
    m = glm::translate(m, glm::vec3(0.0f, 0.5f, 0.0f));
    return m;
}

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
