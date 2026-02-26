#include "scenes/skybox.hpp"
#include "stb_image.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

void Skybox::Load() {
    shader = Shader::LoadShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");

    // Your vertices — 8 corners of a cube
    float vertices[] = {
        -0.5f,  0.5f,  0.5f,  // 0: front top left
         0.5f,  0.5f,  0.5f,  // 1: front top right
         0.5f, -0.5f,  0.5f,  // 2: front bottom right
        -0.5f, -0.5f,  0.5f,  // 3: front bottom left
        -0.5f,  0.5f, -0.5f,  // 4: back top left
         0.5f,  0.5f, -0.5f,  // 5: back top right
         0.5f, -0.5f, -0.5f,  // 6: back bottom right
        -0.5f, -0.5f, -0.5f,  // 7: back bottom left
    };

    // Your indices — 6 faces × 2 triangles
    unsigned int indices[] = {
        0, 1, 2,  2, 3, 0,  // front
        4, 5, 6,  6, 7, 4,  // back
        0, 1, 5,  5, 4, 0,  // top
        3, 2, 6,  6, 7, 3,  // bottom
        1, 5, 6,  6, 2, 1,  // right
        0, 4, 7,  7, 3, 0,  // left
    };

    // Your GPU upload code (with the fixes applied)
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindVertexArray(0);

    // Your cubemap loader
    cubemapTexture = LoadCubemap();
}

void Skybox::Render(const glm::mat4& view, const glm::mat4& projection) {
    // Depth trick: skybox is always "behind" everything
    glDepthFunc(GL_LEQUAL);

    glUseProgram(shader.programID);

    // Strip translation from view matrix (your key learning!)
    glm::mat4 skyboxView = glm::mat4(glm::mat3(view));

    int viewLoc = glGetUniformLocation(shader.programID, "uView");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(skyboxView));

    int projLoc = glGetUniformLocation(shader.programID, "uProjection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glDepthFunc(GL_LESS);  // restore default
}

void Skybox::Unload() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &cubemapTexture);
    shader.Unload();
    VAO = VBO = EBO = cubemapTexture = 0;
}

unsigned int Skybox::LoadCubemap() {
    // Your cubemap loading code
    std::string faces[] = {
        "resources/textures/skybox/right.png",
        "resources/textures/skybox/left.png",
        "resources/textures/skybox/top.png",
        "resources/textures/skybox/bottom.png",
        "resources/textures/skybox/front.png",
        "resources/textures/skybox/back.png"
    };

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (int i = 0; i < 6; i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        } else {
            std::cout << "ERROR::SKYBOX::FAILED_TO_LOAD: " << faces[i] << std::endl;
        }
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
