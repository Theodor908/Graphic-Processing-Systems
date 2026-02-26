#pragma once
#include <glm/glm.hpp>

struct DirectionalLight {
    glm::vec3 direction = glm::vec3(-0.5f, -1.0f, -0.3f);
    glm::vec3 color = glm::vec3(1.0f, 0.95f, 0.8f);
    float intensity = 1.0f;
};

struct PointLight {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    // Attenuation: 1 / (constant + linear*d + quadratic*d^2)
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
};

struct SpotLight {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    float cutOff = glm::cos(glm::radians(25.0f));       // inner cone
    float outerCutOff = glm::cos(glm::radians(35.0f));   // outer cone (soft edge)
    float range = 50.0f;  // attenuation range
};

#define MAX_SPOT_LIGHTS 4
#define MAX_POINT_LIGHTS 8
#define MAX_POINT_SHADOW_LIGHTS 3
