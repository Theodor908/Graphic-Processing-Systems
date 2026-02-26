#include "scenes/p1_scene.hpp"
#include "glad.h"
#include "glfw3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void P1Scene::Load() {
    shader = Shader::LoadShader("resources/shaders/cube.vs", "resources/shaders/cube.fs");

    // Cube vertices: 6 faces × 4 vertices, each with position + color
    float vertices[] = {
        -0.5f, -0.5f,  0.5f,   0.9f, 0.2f, 0.2f,
         0.5f, -0.5f,  0.5f,   0.9f, 0.2f, 0.2f,
         0.5f,  0.5f,  0.5f,   0.9f, 0.2f, 0.2f,
        -0.5f,  0.5f,  0.5f,   0.9f, 0.2f, 0.2f,

        -0.5f, -0.5f, -0.5f,   0.2f, 0.8f, 0.2f,
         0.5f, -0.5f, -0.5f,   0.2f, 0.8f, 0.2f,
         0.5f,  0.5f, -0.5f,   0.2f, 0.8f, 0.2f,
        -0.5f,  0.5f, -0.5f,   0.2f, 0.8f, 0.2f,

        -0.5f,  0.5f, -0.5f,   0.2f, 0.2f, 0.9f,
         0.5f,  0.5f, -0.5f,   0.2f, 0.2f, 0.9f,
         0.5f,  0.5f,  0.5f,   0.2f, 0.2f, 0.9f,
        -0.5f,  0.5f,  0.5f,   0.2f, 0.2f, 0.9f,

        -0.5f, -0.5f, -0.5f,   0.9f, 0.9f, 0.2f,
         0.5f, -0.5f, -0.5f,   0.9f, 0.9f, 0.2f,
         0.5f, -0.5f,  0.5f,   0.9f, 0.9f, 0.2f,
        -0.5f, -0.5f,  0.5f,   0.9f, 0.9f, 0.2f,

         0.5f, -0.5f, -0.5f,   0.9f, 0.2f, 0.9f,
         0.5f,  0.5f, -0.5f,   0.9f, 0.2f, 0.9f,
         0.5f,  0.5f,  0.5f,   0.9f, 0.2f, 0.9f,
         0.5f, -0.5f,  0.5f,   0.9f, 0.2f, 0.9f,

        -0.5f, -0.5f, -0.5f,   0.2f, 0.9f, 0.9f,
        -0.5f,  0.5f, -0.5f,   0.2f, 0.9f, 0.9f,
        -0.5f,  0.5f,  0.5f,   0.2f, 0.9f, 0.9f,
        -0.5f, -0.5f,  0.5f,   0.2f, 0.9f, 0.9f,
    };

    unsigned int indices[] = {
         0,  1,  2,   2,  3,  0,
         4,  5,  6,   6,  7,  4,
         8,  9, 10,  10, 11,  8,
        12, 13, 14,  14, 15, 12,
        16, 17, 18,  18, 19, 16,
        20, 21, 22,  22, 23, 20,
    };

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindVertexArray(0);

    // Load skybox
    skybox.Load();

    glEnable(GL_DEPTH_TEST);
    glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void P1Scene::Update() {
    shader.ReloadFromFile();
    rotationAngle += 0.5f;

    int width, height;
    GLFWwindow* window = glfwGetCurrentContext();
    glfwGetWindowSize(window, &width, &height);
    float centerX = width / 2.0f;
    float centerY = height / 2.0f;

    double mouseX, mouseY;

    glfwGetCursorPos(glfwGetCurrentContext(), &mouseX, &mouseY);

    float deltaX = (float)mouseX - centerX;
    float deltaY = centerY - (float)mouseY;  // reversed Y

    glfwSetCursorPos(window, centerX, centerY);

    float sensitivity = 0.1f;
    yaw += deltaX * sensitivity;
    pitch += deltaY * sensitivity;

    if(pitch > 120.0f) pitch = 120.0f;
    if(pitch < -120.0f) pitch = -120.0f;

    cameraDir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraDir.y = sin(glm::radians(pitch));
    cameraDir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraDir = glm::normalize(cameraDir);
}

void P1Scene::Render() {
    // Shared view & projection matrices
    glm::mat4 view = glm::lookAt(
        cameraPos,
        cameraPos + cameraDir,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        800.0f / 600.0f,
        0.1f,
        100.0f
    );

    // 1. Draw skybox first (behind everything)
    skybox.Render(view, projection);

    // 2. Draw the rotating cube
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.5f, 1.0f, 0.0f));
    glm::mat4 mvp = projection * view * model;

    glUseProgram(shader.programID);
    int mvpLoc = glGetUniformLocation(shader.programID, "uMVP");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

void P1Scene::Unload() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    shader.Unload();
    skybox.Unload();
    VAO = VBO = EBO = 0;
    rotationAngle = 0.0f;
    glDisable(GL_DEPTH_TEST);
    glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}
