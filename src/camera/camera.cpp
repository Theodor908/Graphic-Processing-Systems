#include "camera/camera.hpp"
#include <cmath>

Camera::Camera(glm::vec3 startPos) : position(startPos) {
    direction = glm::vec3(0.0f, 0.0f, -1.0f);
}

void Camera::Update(GLFWwindow* window, float deltaTime) {
    ProcessMouse(window);
    ProcessKeyboard(window, deltaTime);
}

glm::mat4 Camera::GetViewMatrix() const {
    glm::mat4 rollMat = glm::rotate(glm::mat4(1.0f), glm::radians(roll), direction);
    glm::vec3 rolledUp = glm::vec3(rollMat * glm::vec4(glm::vec3(0.0f, 1.0f, 0.0f), 0.0f));
    return glm::lookAt(position, position + direction, rolledUp);
}

void Camera::ProcessMouse(GLFWwindow* window) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    float centerX = width / 2.0f;
    float centerY = height / 2.0f;

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    float deltaX = (float)mouseX - centerX;
    float deltaY = centerY - (float)mouseY;

    glfwSetCursorPos(window, centerX, centerY);

    yaw   += deltaX * sensitivity;
    pitch += deltaY * sensitivity;

    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction = glm::normalize(direction);
}

void Camera::ProcessKeyboard(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        speed = glm::min(speed + speedAcceleration * deltaTime, maxSpeed);
    else
        speed = 5.0f;

    float velocity = speed * deltaTime;

    // Apply roll to get correct right/up vectors for movement
    glm::mat4 rollMat = glm::rotate(glm::mat4(1.0f), glm::radians(roll), direction);
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 rolledUp = glm::vec3(rollMat * glm::vec4(worldUp, 0.0f));
    glm::vec3 right = glm::normalize(glm::cross(direction, rolledUp));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        position += direction * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        position -= direction * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        position -= right * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        position += right * velocity;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        position += rolledUp * velocity;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        position -= rolledUp * velocity;

    // Roll: Q = roll left, E = roll right
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        roll += rollSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        roll -= rollSpeed * deltaTime;
}
