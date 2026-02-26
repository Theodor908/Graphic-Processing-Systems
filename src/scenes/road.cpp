#include "scenes/road.hpp"
#include "stb_image.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

void Road::Load() {
    shader = Shader::LoadShader("resources/shaders/road.vs", "resources/shaders/road.fs");
    GenerateGeometry();
    texture = LoadTexture("resources/textures/road/road.jpg");
}

void Road::GenerateGeometry() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    segments = 64;
    outerRadiusX = 40.0f;
    outerRadiusZ = 30.0f;
    roadWidth = 5.0f;
    roadY = 1.02f;

    for(int i = 0; i <= segments; i++)
    {
        float angle = (i / (float)segments) * 2 * PI;
        float outerX = outerRadiusX * cos(angle);
        float outerZ = outerRadiusZ * sin(angle);
        float innerX = (outerRadiusX - roadWidth) * cos(angle);
        float innerZ = (outerRadiusZ - roadWidth) * sin(angle);

        // Outer vertex
        vertices.push_back(outerX);
        vertices.push_back(roadY);
        vertices.push_back(outerZ);
        vertices.push_back(0.0f);  // normal x
        vertices.push_back(1.0f);  // normal y
        vertices.push_back(0.0f);  // normal z
        vertices.push_back(1.0f);  // u
        vertices.push_back((float)i / segments * 4.0f);  // v
        // Inner vertex
        vertices.push_back(innerX);
        vertices.push_back(roadY);
        vertices.push_back(innerZ);
        vertices.push_back(0.0f);  // normal x
        vertices.push_back(1.0f);  // normal y
        vertices.push_back(0.0f);  // normal z
        vertices.push_back(0.0f);  // u
        vertices.push_back((float)i / segments * 4.0f);  // v
    }

    for(int i = 0; i < segments; i++)
    {
        indices.push_back(i * 2);       // outer current
        indices.push_back(i * 2 + 1);   // inner current
        indices.push_back((i + 1) * 2); // outer next

        indices.push_back(i * 2 + 1);   // inner current
        indices.push_back((i + 1) * 2 + 1); // inner next
        indices.push_back((i + 1) * 2); // outer next
    }

    indexCount = (int)indices.size();


    // --- GPU upload (don't modify below) ---
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position: layout 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal: layout 1
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // UV: layout 2
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Road::Render(const glm::mat4& view, const glm::mat4& projection) {
    glUseProgram(shader.programID);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 mvp = projection * view * model;

    int mvpLoc = glGetUniformLocation(shader.programID, "uMVP");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Road::DrawGeometry() {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Road::Unload() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &texture);
    shader.Unload();
    VAO = VBO = EBO = texture = 0;
    indexCount = 0;
}

unsigned int Road::LoadTexture(const std::string& path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "ERROR::ROAD::FAILED_TO_LOAD_TEXTURE: " << path << std::endl;
    }
    stbi_image_free(data);

    return textureID;
}
