#pragma once
#include "glfw3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    float yaw = -90.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
    float speed = 5.0f;
    float maxSpeed = 25.0f;
    float speedAcceleration = 10.0f;
    float sensitivity = 0.1f;
    float rollSpeed = 45.0f;

    Camera(glm::vec3 startPos = glm::vec3(0.0f, 2.0f, 3.0f));

    void Update(GLFWwindow* window, float deltaTime);
    glm::mat4 GetViewMatrix() const;

private:
    void ProcessMouse(GLFWwindow* window);
    void ProcessKeyboard(GLFWwindow* window, float deltaTime);
};
