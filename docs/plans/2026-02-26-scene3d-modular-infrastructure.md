# Scene3D Modular Infrastructure Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Extract camera, skybox, terrain, cursor, and timing boilerplate into a Scene3D base class so scenes only implement their unique logic.

**Architecture:** Scene3D inherits from Scene and implements Load/Update/Render/Unload as `final`, delegating to On* hooks. A static Time struct provides Unity-style deltaTime. Terrain accepts a TerrainGenerator strategy interface for height generation. The ImGui click-stealing bug is fixed in Scene3D's cursor logic.

**Tech Stack:** C++17, OpenGL 3.3 Core, GLFW, GLM, ImGui

---

### Task 1: Create Time Utility

**Files:**
- Create: `include/utils/time.hpp`
- Create: `src/utils/time.cpp`

**Step 1: Create the header**

Create `include/utils/time.hpp`:
```cpp
#pragma once
#include "glfw3.h"

struct Time {
    static float deltaTime;
    static float time;
    static float lastFrameTime;

    static void Update() {
        float currentTime = (float)glfwGetTime();
        deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        time = currentTime;
    }

    static void Reset() {
        lastFrameTime = (float)glfwGetTime();
        deltaTime = 0.0f;
        time = lastFrameTime;
    }
};
```

**Step 2: Create the source file**

Create `src/utils/time.cpp`:
```cpp
#include "utils/time.hpp"

float Time::deltaTime = 0.0f;
float Time::time = 0.0f;
float Time::lastFrameTime = 0.0f;
```

**Step 3: Build and verify**

Run: `cmake --build build`
Expected: Compiles with no errors

**Step 4: Commit**

```bash
git add include/utils/time.hpp src/utils/time.cpp
git commit -m "feat: add Unity-style Time utility with static deltaTime"
```

---

### Task 2: Create TerrainGenerator Interface

**Files:**
- Create: `include/scenes/terrain_generator.hpp`

**Step 1: Create the header**

Create `include/scenes/terrain_generator.hpp`:
```cpp
#pragma once

class TerrainGenerator {
public:
    virtual ~TerrainGenerator() = default;
    virtual float GetHeight(float x, float z) const = 0;
};

class FlatGenerator : public TerrainGenerator {
    float height;
public:
    FlatGenerator(float h = 1.0f) : height(h) {}
    float GetHeight(float x, float z) const override { return height; }
};
```

**Step 2: Build and verify**

Run: `cmake --build build`
Expected: Compiles (header-only, no linker changes)

**Step 3: Commit**

```bash
git add include/scenes/terrain_generator.hpp
git commit -m "feat: add TerrainGenerator interface with FlatGenerator default"
```

---

### Task 3: Update Terrain to Use Generator

**Files:**
- Modify: `include/scenes/terrain.hpp`
- Modify: `src/scenes/terrain.cpp:9-31` (Load method, vertex generation loop)

**Step 1: Update terrain header**

In `include/scenes/terrain.hpp`, add the generator include and overloaded Load:

Replace the entire file with:
```cpp
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
```

Key changes:
- Added `#include "scenes/terrain_generator.hpp"`
- Added `void Load(TerrainGenerator* generator)` overload
- Added `void GenerateMesh(TerrainGenerator* generator)` private method
- Removed `float heightScale = 1.0f` (generator handles height now)

**Step 2: Update terrain source**

In `src/scenes/terrain.cpp`, refactor Load to use generator:

Replace the entire file with:
```cpp
#include "scenes/terrain.hpp"
#include "stb_image.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>

void Terrain::Load() {
    FlatGenerator flat(1.0f);
    Load(&flat);
}

void Terrain::Load(TerrainGenerator* generator) {
    shader = Shader::LoadShader("resources/shaders/terrain.vs", "resources/shaders/terrain.fs");
    GenerateMesh(generator);
    texture = LoadTexture("resources/textures/terrain/terrain.jpg");
}

void Terrain::GenerateMesh(TerrainGenerator* generator) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (int z = 0; z <= gridSize; z++) {
        for (int x = 0; x <= gridSize; x++) {
            float xPos = (float)x - gridSize / 2.0f;
            float zPos = (float)z - gridSize / 2.0f;
            float yPos = generator->GetHeight(xPos, zPos);

            float u = (float)x / gridSize;
            float v = (float)z / gridSize;

            vertices.push_back(xPos);
            vertices.push_back(yPos);
            vertices.push_back(zPos);
            vertices.push_back(u);
            vertices.push_back(v);
        }
    }

    for (int z = 0; z < gridSize; z++) {
        for (int x = 0; x < gridSize; x++) {
            int topLeft = z * (gridSize + 1) + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * (gridSize + 1) + x;
            int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    indexCount = (int)indices.size();

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Terrain::Render(const glm::mat4& view, const glm::mat4& projection) {
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

void Terrain::Unload() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &texture);
    shader.Unload();
    VAO = VBO = EBO = texture = 0;
    indexCount = 0;
}

unsigned int Terrain::LoadTexture(const std::string& path) {
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
        std::cout << "ERROR::TERRAIN::FAILED_TO_LOAD_TEXTURE: " << path << std::endl;
    }
    stbi_image_free(data);

    return textureID;
}
```

Key change: `float yPos = generator->GetHeight(xPos, zPos);` replaces `float yPos = heightScale;`

**Step 3: Build and verify**

Run: `cmake --build build`
Expected: Compiles. Run the app — P1 scene terrain should look identical (FlatGenerator defaults to height 1.0).

**Step 4: Commit**

```bash
git add include/scenes/terrain.hpp src/scenes/terrain.cpp
git commit -m "feat: terrain uses TerrainGenerator strategy for height generation"
```

---

### Task 4: Create Scene3D Base Class

**Files:**
- Create: `include/scenes/scene3d.hpp`
- Create: `src/scenes/scene3d.cpp`

**Step 1: Create the header**

Create `include/scenes/scene3d.hpp`:
```cpp
#pragma once
#include "scenes/scene.hpp"
#include "scenes/skybox.hpp"
#include "scenes/terrain.hpp"
#include "scenes/terrain_generator.hpp"
#include "camera/camera.hpp"
#include <glm/glm.hpp>

class Scene3D : public Scene {
public:
    Scene3D(const std::string& name,
            glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 3.0f),
            float fov = 45.0f,
            float nearPlane = 0.1f,
            float farPlane = 100.0f);

protected:
    Camera camera;
    Skybox skybox;
    Terrain terrain;

    float fov;
    float nearPlane;
    float farPlane;
    bool cursorLocked = true;

    virtual void OnLoad() = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender(const glm::mat4& view, const glm::mat4& projection) = 0;
    virtual void OnUnload() = 0;

    // Optional: override to provide a custom terrain generator
    virtual TerrainGenerator* GetTerrainGenerator() { return nullptr; }

private:
    void Load() override final;
    void Update() override final;
    void Render() override final;
    void Unload() override final;
};
```

**Step 2: Create the source file**

Create `src/scenes/scene3d.cpp`:
```cpp
#include "scenes/scene3d.hpp"
#include "utils/time.hpp"
#include "glad.h"
#include "glfw3.h"
#include "imgui.h"

#include <glm/gtc/matrix_transform.hpp>

Scene3D::Scene3D(const std::string& name, glm::vec3 cameraPos, float fov, float nearPlane, float farPlane)
    : Scene(name), camera(cameraPos), fov(fov), nearPlane(nearPlane), farPlane(farPlane) {}

void Scene3D::Load() {
    glEnable(GL_DEPTH_TEST);

    GLFWwindow* window = glfwGetCurrentContext();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    cursorLocked = true;

    Time::Reset();

    skybox.Load();

    TerrainGenerator* gen = GetTerrainGenerator();
    if (gen)
        terrain.Load(gen);
    else
        terrain.Load();

    OnLoad();
}

void Scene3D::Update() {
    Time::Update();

    GLFWwindow* window = glfwGetCurrentContext();

    if (glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && cursorLocked) {
        cursorLocked = false;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS
        && !cursorLocked
        && !ImGui::GetIO().WantCaptureMouse) {
        cursorLocked = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    if (cursorLocked)
        camera.Update(window, Time::deltaTime);

    OnUpdate();
}

void Scene3D::Render() {
    glm::mat4 view = camera.GetViewMatrix();

    glm::mat4 projection = glm::perspective(
        glm::radians(fov),
        800.0f / 600.0f,
        nearPlane,
        farPlane
    );

    skybox.Render(view, projection);
    terrain.Render(view, projection);

    OnRender(view, projection);
}

void Scene3D::Unload() {
    OnUnload();
    skybox.Unload();
    terrain.Unload();
    glDisable(GL_DEPTH_TEST);
    glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    cursorLocked = true;
}
```

**Step 3: Build and verify**

Run: `cmake --build build`
Expected: Compiles with no errors (Scene3D not yet used by any scene)

**Step 4: Commit**

```bash
git add include/scenes/scene3d.hpp src/scenes/scene3d.cpp
git commit -m "feat: add Scene3D base class with camera, skybox, terrain, cursor, Time"
```

---

### Task 5: Refactor P1Scene to Use Scene3D

**Files:**
- Modify: `include/scenes/p1_scene.hpp`
- Modify: `src/scenes/p1_scene.cpp`

**Step 1: Update P1Scene header**

Replace `include/scenes/p1_scene.hpp` with:
```cpp
#pragma once
#include "scenes/scene3d.hpp"
#include "shaders/shader.hpp"

class P1Scene : public Scene3D {
public:
    P1Scene() : Scene3D("Skybox", glm::vec3(0.0f, 2.0f, 3.0f)) {}

    void OnLoad() override;
    void OnUpdate() override;
    void OnRender(const glm::mat4& view, const glm::mat4& projection) override;
    void OnUnload() override;

private:
    Shader shader;
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    float rotationAngle = 0.0f;
};
```

Removed: Camera, Skybox, Terrain, lastFrameTime, cursorLocked — all handled by Scene3D now.

**Step 2: Update P1Scene source**

Replace `src/scenes/p1_scene.cpp` with:
```cpp
#include "scenes/p1_scene.hpp"
#include "utils/time.hpp"
#include "glad.h"
#include "glfw3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void P1Scene::OnLoad() {
    shader = Shader::LoadShader("resources/shaders/cube.vs", "resources/shaders/cube.fs");

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
}

void P1Scene::OnUpdate() {
    shader.ReloadFromFile();
    rotationAngle += 0.5f;
}

void P1Scene::OnRender(const glm::mat4& view, const glm::mat4& projection) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.5f, 1.0f, 0.0f));
    glm::mat4 mvp = projection * view * model;

    glUseProgram(shader.programID);
    int mvpLoc = glGetUniformLocation(shader.programID, "uMVP");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

void P1Scene::OnUnload() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    shader.Unload();
    VAO = VBO = EBO = 0;
    rotationAngle = 0.0f;
}
```

**Step 3: Build and run**

Run: `cmake --build build && ./build/opengl-imgui-cmake-template`
Expected: P1 scene works identically — skybox, terrain, rotating cube, camera. ESC unlocks cursor, clicking ImGui tabs works without re-locking.

**Step 4: Verify ImGui tab fix**

Test: Press ESC to unlock cursor, then click on "Scene 2" tab.
Expected: Tab switches to Scene 2 without snapping back to Scene 1.

**Step 5: Commit**

```bash
git add include/scenes/p1_scene.hpp src/scenes/p1_scene.cpp
git commit -m "refactor: P1Scene uses Scene3D base class, fix ImGui tab click bug"
```

---

## Summary of Changes

| Task | What | Files |
|------|------|-------|
| 1 | Time utility | `include/utils/time.hpp`, `src/utils/time.cpp` |
| 2 | TerrainGenerator interface | `include/scenes/terrain_generator.hpp` |
| 3 | Terrain uses generator | `include/scenes/terrain.hpp`, `src/scenes/terrain.cpp` |
| 4 | Scene3D base class | `include/scenes/scene3d.hpp`, `src/scenes/scene3d.cpp` |
| 5 | P1Scene refactor + tab fix | `include/scenes/p1_scene.hpp`, `src/scenes/p1_scene.cpp` |

## Verification Checklist

- [ ] P1 scene renders skybox, terrain, rotating cube identically to before
- [ ] Camera WASD + mouse look works
- [ ] ESC unlocks cursor
- [ ] Clicking ImGui tabs while cursor is unlocked switches scenes (no snap-back)
- [ ] `Time::deltaTime` is accessible from OnUpdate
- [ ] `terrain.Load()` with no args produces flat plane at y=1.0
- [ ] `terrain.Load(&customGen)` uses custom height function
