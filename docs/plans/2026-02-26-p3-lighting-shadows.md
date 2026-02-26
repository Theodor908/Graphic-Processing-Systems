# P3: Camera Roll, Configurable Scene3D, Lighting & Shadows Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add camera roll, make Scene3D configurable (opt-in skybox/terrain/lighting), implement a directional sun with shadows, and spot streetlights with shadow maps.

**Architecture:** Scene3DConfig struct controls which components are active. LightingSystem manages lights and shadow maps independently. Meshes gain normals and a DrawGeometry() method for shader-agnostic rendering. Shadow mapping uses depth FBOs (1 for sun, 1 per spot light). Blinn-Phong lit shader samples all shadow maps.

**Tech Stack:** C++17, OpenGL 3.3 Core, GLFW, GLM, ImGui, stb_image

---

### Task 1: Add Camera Roll

**Files:**
- Modify: `include/camera/camera.hpp`
- Modify: `src/camera/camera.cpp`

**Step 1: Update camera header**

In `include/camera/camera.hpp`, add roll member and roll speed:

```cpp
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
    float rollSpeed = 45.0f;  // degrees per second

    Camera(glm::vec3 startPos = glm::vec3(0.0f, 2.0f, 3.0f));

    void Update(GLFWwindow* window, float deltaTime);
    glm::mat4 GetViewMatrix() const;

private:
    void ProcessMouse(GLFWwindow* window);
    void ProcessKeyboard(GLFWwindow* window, float deltaTime);
};
```

**Step 2: Update camera source**

In `src/camera/camera.cpp`, modify `ProcessKeyboard` to handle Q/E roll and update `GetViewMatrix` to apply roll:

```cpp
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
    // Apply roll by rotating the up vector around the look direction
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
```

**Step 3: Build and verify**

Run: `cd "C:/Users/vasil/Desktop/Graphic-Processing-Systems" && cmake --build build`
Test: Run app, hold Q or E — camera should tilt. WASD/Space/Ctrl movement should respect the tilt.

**Step 4: Commit**

```bash
git add include/camera/camera.hpp src/camera/camera.cpp
git commit -m "feat: add camera roll with Q/E keys, all 3 rotation axes"
```

---

### Task 2: Make Scene3D Configurable with Scene3DConfig

**Files:**
- Modify: `include/scenes/scene3d.hpp`
- Modify: `src/scenes/scene3d.cpp`
- Modify: `include/scenes/p1_scene.hpp` (update constructor)
- Modify: `include/scenes/p2_scene.hpp` (update constructor)

**Step 1: Update Scene3D header**

Replace `include/scenes/scene3d.hpp`:

```cpp
#pragma once
#include "scenes/scene.hpp"
#include "scenes/skybox.hpp"
#include "scenes/terrain.hpp"
#include "scenes/terrain_generator.hpp"
#include "camera/camera.hpp"
#include <glm/glm.hpp>
#include <string>

struct Scene3DConfig {
    std::string name = "Scene";
    glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 3.0f);
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    bool useSkybox = true;
    std::string skyboxPath = "resources/textures/skybox/";

    bool useTerrain = true;
    TerrainGenerator* terrainGenerator = nullptr;

    bool useLighting = false;
};

class Scene3D : public Scene {
public:
    Scene3D(const Scene3DConfig& config);

protected:
    Camera camera;
    Skybox skybox;
    Terrain terrain;
    Scene3DConfig config;
    bool cursorLocked = true;

    virtual void OnLoad() = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender(const glm::mat4& view, const glm::mat4& projection) = 0;
    virtual void OnUnload() = 0;

private:
    void Load() override final;
    void Update() override final;
    void Render() override final;
    void Unload() override final;
};
```

Note: Removed `GetTerrainGenerator()` virtual — replaced by `config.terrainGenerator`.
Removed individual `fov/nearPlane/farPlane` — stored in `config`.

**Step 2: Update Scene3D source**

Replace `src/scenes/scene3d.cpp`:

```cpp
#include "scenes/scene3d.hpp"
#include "utils/time.hpp"
#include "glad.h"
#include "glfw3.h"
#include "imgui.h"

#include <glm/gtc/matrix_transform.hpp>

Scene3D::Scene3D(const Scene3DConfig& cfg)
    : Scene(cfg.name), camera(cfg.cameraPos), config(cfg) {}

void Scene3D::Load() {
    glEnable(GL_DEPTH_TEST);

    GLFWwindow* window = glfwGetCurrentContext();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    cursorLocked = true;

    Time::Reset();

    if (config.useSkybox)
        skybox.Load();

    if (config.useTerrain) {
        if (config.terrainGenerator)
            terrain.Load(config.terrainGenerator);
        else
            terrain.Load();
    }

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
        glm::radians(config.fov),
        800.0f / 600.0f,
        config.nearPlane,
        config.farPlane
    );

    if (config.useSkybox)
        skybox.Render(view, projection);

    if (config.useTerrain)
        terrain.Render(view, projection);

    OnRender(view, projection);
}

void Scene3D::Unload() {
    OnUnload();

    if (config.useSkybox)
        skybox.Unload();

    if (config.useTerrain)
        terrain.Unload();

    glDisable(GL_DEPTH_TEST);
    glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    cursorLocked = true;
}
```

**Step 3: Update P1Scene constructor**

In `include/scenes/p1_scene.hpp`, change:
```cpp
P1Scene() : Scene3D("Skybox", glm::vec3(0.0f, 2.0f, 3.0f)) {}
```
to:
```cpp
P1Scene() : Scene3D({.name = "Skybox", .cameraPos = glm::vec3(0.0f, 2.0f, 3.0f)}) {}
```

**Step 4: Update P2Scene constructor**

In `include/scenes/p2_scene.hpp`, change:
```cpp
P2Scene() : Scene3D("Street", glm::vec3(0.0f, 15.0f, 50.0f)) {}
```
to:
```cpp
P2Scene() : Scene3D({.name = "Street", .cameraPos = glm::vec3(0.0f, 15.0f, 50.0f)}) {}
```

**Step 5: Build and verify**

Run: `cmake --build build`
Test: P1 and P2 scenes should work identically to before.

**Step 6: Commit**

```bash
git add include/scenes/scene3d.hpp src/scenes/scene3d.cpp include/scenes/p1_scene.hpp include/scenes/p2_scene.hpp
git commit -m "refactor: Scene3D uses Scene3DConfig for configurable skybox/terrain/lighting"
```

---

### Task 3: Add Normals + DrawGeometry to All Meshes

This task adds normal vectors to terrain, road, and static object vertex data, and adds a `DrawGeometry()` method to each for shader-agnostic rendering during shadow passes.

**Files:**
- Modify: `include/scenes/terrain.hpp`
- Modify: `src/scenes/terrain.cpp`
- Modify: `include/scenes/road.hpp`
- Modify: `src/scenes/road.cpp`
- Modify: `include/scenes/static_object.hpp`
- Modify: `src/scenes/static_object.cpp`
- Modify: `resources/shaders/terrain.vs`
- Modify: `resources/shaders/road.vs`
- Modify: `resources/shaders/object.vs`

**Step 1: Update terrain**

In `include/scenes/terrain.hpp`, add:
```cpp
    void DrawGeometry();  // just binds VAO and draws — no shader setup
```
under the public section, after `void Unload();`.

In `src/scenes/terrain.cpp`, change vertex format from 5 floats (pos+uv) to 8 floats (pos+normal+uv).

Replace `GenerateMesh` method:
```cpp
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

            // Position
            vertices.push_back(xPos);
            vertices.push_back(yPos);
            vertices.push_back(zPos);

            // Normal — compute from cross product of neighbors for hilly terrain
            float hL = generator->GetHeight(xPos - 1, zPos);
            float hR = generator->GetHeight(xPos + 1, zPos);
            float hD = generator->GetHeight(xPos, zPos - 1);
            float hU = generator->GetHeight(xPos, zPos + 1);
            glm::vec3 normal = glm::normalize(glm::vec3(hL - hR, 2.0f, hD - hU));
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);

            // UV
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

    // Position: layout 0, 3 floats, stride = 8 floats
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal: layout 1, 3 floats
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // UV: layout 2, 2 floats
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}
```

Add DrawGeometry method:
```cpp
void Terrain::DrawGeometry() {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
```

Update `terrain.vs` — UV is now at layout 2:
```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

uniform mat4 uMVP;

out vec2 texCoord;

void main()
{
    texCoord = aUV;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
```

**Step 2: Update road**

In `include/scenes/road.hpp`, add `void DrawGeometry();` to public section.

In `src/scenes/road.cpp`, update `GenerateGeometry` — add normal (0,1,0) for flat road, stride = 8:

Each vertex becomes: x, y, z, nx, ny, nz, u, v

For the road, all normals are (0, 1, 0) since it's flat.

In the vertex push loop, after pushing z, add:
```cpp
        // Normal (flat road points up)
        vertices.push_back(0.0f);
        vertices.push_back(1.0f);
        vertices.push_back(0.0f);
```
for BOTH inner and outer vertices.

Update GPU layout to stride 8, with:
- layout 0: position (3 floats, offset 0)
- layout 1: normal (3 floats, offset 3)
- layout 2: UV (2 floats, offset 6)

Add:
```cpp
void Road::DrawGeometry() {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
```

Update `road.vs`:
```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

uniform mat4 uMVP;

out vec2 texCoord;

void main()
{
    texCoord = aUV;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
```

**Step 3: Update static objects**

In `include/scenes/static_object.hpp`, add `void DrawGeometry(const std::vector<ObjectInstance>& objects);` to public section.

In `src/scenes/static_object.cpp`, update `CreateCubeMesh` — each vertex becomes 8 floats: x, y, z, nx, ny, nz, u, v.

Cube face normals:
- Front:  (0, 0, 1)
- Back:   (0, 0, -1)
- Top:    (0, 1, 0)
- Bottom: (0, -1, 0)
- Right:  (1, 0, 0)
- Left:   (-1, 0, 0)

```cpp
void StaticObjectRenderer::CreateCubeMesh() {
    // Textured unit cube with normals: 24 vertices (4 per face)
    // Each vertex: x, y, z, nx, ny, nz, u, v
    float vertices[] = {
        // Front face (normal: 0, 0, 1)
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
        // Back face (normal: 0, 0, -1)
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,-1.0f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f,-1.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f,-1.0f,  0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f,-1.0f,  1.0f, 1.0f,
        // Top face (normal: 0, 1, 0)
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
        // Bottom face (normal: 0, -1, 0)
        -0.5f, -0.5f, -0.5f,  0.0f,-1.0f, 0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,-1.0f, 0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,-1.0f, 0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,-1.0f, 0.0f,  0.0f, 0.0f,
        // Right face (normal: 1, 0, 0)
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        // Left face (normal: -1, 0, 0)
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
    };

    unsigned int indices[] = {
         0,  1,  2,   2,  3,  0,
         4,  5,  6,   6,  7,  4,
         8,  9, 10,  10, 11,  8,
        12, 13, 14,  14, 15, 12,
        16, 17, 18,  18, 19, 16,
        20, 21, 22,  22, 23, 20,
    };

    glGenVertexArrays(1, &cubeVAO);
    glBindVertexArray(cubeVAO);

    glGenBuffers(1, &cubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &cubeEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

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
```

Update `object.vs`:
```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

uniform mat4 uMVP;

out vec2 texCoord;

void main()
{
    texCoord = aUV;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
```

Add DrawGeometry (no per-object model matrix — caller handles this):
```cpp
void StaticObjectRenderer::DrawGeometry(const std::vector<ObjectInstance>& objects) {
    glBindVertexArray(cubeVAO);
    // Caller must set shader and per-object uniforms before each draw
    // This just exposes the VAO binding for external rendering
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
```

Note: For static objects in the shadow pass, Scene3D will loop through objects, set model matrix on shadow shader, then call DrawGeometry. So DrawGeometry just draws one cube — the caller loops.

Actually, let's simplify. Rename to `BindAndDraw()` — draws one cube. Caller loops over objects.
```cpp
void StaticObjectRenderer::BindAndDraw() {
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
```

**Step 4: Build and verify**

Run: `cmake --build build`
Test: P1, P2 scenes should render identically (unlit shaders ignore the normal attribute — layout 1 is declared in VS but unused in FS).

**Step 5: Commit**

```bash
git add include/scenes/terrain.hpp src/scenes/terrain.cpp include/scenes/road.hpp src/scenes/road.cpp include/scenes/static_object.hpp src/scenes/static_object.cpp resources/shaders/terrain.vs resources/shaders/road.vs resources/shaders/object.vs
git commit -m "feat: add normals to all meshes, add DrawGeometry for shader-agnostic rendering"
```

---

### Task 4: Create Light Structs

**Files:**
- Create: `include/lighting/light.hpp`

**Step 1: Create light types header**

```cpp
#pragma once
#include <glm/glm.hpp>

struct DirectionalLight {
    glm::vec3 direction = glm::vec3(-0.5f, -1.0f, -0.3f);
    glm::vec3 color = glm::vec3(1.0f, 0.95f, 0.8f);
    float intensity = 1.0f;
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
```

**Step 2: Commit**

```bash
git add include/lighting/light.hpp
git commit -m "feat: add DirectionalLight and SpotLight structs"
```

---

### Task 5: Create Shadow and Lit Shaders

**Files:**
- Create: `resources/shaders/shadow.vs`
- Create: `resources/shaders/shadow.fs`
- Create: `resources/shaders/lit.vs`
- Create: `resources/shaders/lit.fs`

**Step 1: Shadow depth shaders**

`resources/shaders/shadow.vs`:
```glsl
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 uLightMVP;

void main()
{
    gl_Position = uLightMVP * vec4(aPos, 1.0);
}
```

`resources/shaders/shadow.fs`:
```glsl
#version 330 core

void main()
{
    // Depth is written automatically — nothing to do
}
```

**Step 2: Lit shaders with Blinn-Phong + shadow sampling**

`resources/shaders/lit.vs`:
```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uSunLightSpaceMVP;

#define MAX_SPOT_LIGHTS 4
uniform mat4 uSpotLightSpaceMVP[MAX_SPOT_LIGHTS];
uniform int uNumSpotLights;

out vec3 fragPos;
out vec3 fragNormal;
out vec2 texCoord;
out vec4 fragPosLightSpace;     // for sun shadow
out vec4 fragPosSpotSpace[MAX_SPOT_LIGHTS]; // for spot shadows

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    fragPos = worldPos.xyz;
    fragNormal = mat3(transpose(inverse(uModel))) * aNormal;
    texCoord = aUV;

    fragPosLightSpace = uSunLightSpaceMVP * vec4(aPos, 1.0);

    for (int i = 0; i < uNumSpotLights; i++) {
        fragPosSpotSpace[i] = uSpotLightSpaceMVP[i] * vec4(aPos, 1.0);
    }

    gl_Position = uProjection * uView * worldPos;
}
```

`resources/shaders/lit.fs`:
```glsl
#version 330 core
out vec4 FragColor;

in vec3 fragPos;
in vec3 fragNormal;
in vec2 texCoord;
in vec4 fragPosLightSpace;

#define MAX_SPOT_LIGHTS 4
in vec4 fragPosSpotSpace[MAX_SPOT_LIGHTS];

// Material
uniform sampler2D uTexture;

// Camera
uniform vec3 uViewPos;

// Directional light (sun)
uniform vec3 uSunDirection;
uniform vec3 uSunColor;
uniform float uSunIntensity;
uniform sampler2D uSunShadowMap;

// Spot lights
uniform int uNumSpotLights;
uniform vec3 uSpotPos[MAX_SPOT_LIGHTS];
uniform vec3 uSpotDir[MAX_SPOT_LIGHTS];
uniform vec3 uSpotColor[MAX_SPOT_LIGHTS];
uniform float uSpotIntensity[MAX_SPOT_LIGHTS];
uniform float uSpotCutOff[MAX_SPOT_LIGHTS];
uniform float uSpotOuterCutOff[MAX_SPOT_LIGHTS];
uniform float uSpotRange[MAX_SPOT_LIGHTS];
uniform sampler2D uSpotShadowMap[MAX_SPOT_LIGHTS];

// Ambient
uniform vec3 uAmbientColor;

float CalcShadow(vec4 fragPosLight, sampler2D shadowMap, vec3 lightDir, vec3 normal)
{
    vec3 projCoords = fragPosLight.xyz / fragPosLight.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    // Bias to reduce shadow acne
    float bias = max(0.005 * (1.0 - dot(normal, -lightDir)), 0.001);

    // PCF (percentage-closer filtering) for soft shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(uViewPos - fragPos);
    vec3 texColor = texture(uTexture, texCoord).rgb;

    // Ambient
    vec3 ambient = uAmbientColor * texColor;

    // --- Directional light (sun) ---
    vec3 sunDir = normalize(-uSunDirection);
    float sunDiff = max(dot(normal, sunDir), 0.0);
    vec3 sunHalf = normalize(sunDir + viewDir);
    float sunSpec = pow(max(dot(normal, sunHalf), 0.0), 32.0);

    float sunShadow = CalcShadow(fragPosLightSpace, uSunShadowMap, uSunDirection, normal);

    vec3 sunResult = (1.0 - sunShadow) * (sunDiff * texColor + sunSpec * vec3(0.3))
                     * uSunColor * uSunIntensity;

    // --- Spot lights ---
    vec3 spotResult = vec3(0.0);
    for (int i = 0; i < uNumSpotLights; i++) {
        vec3 lightVec = uSpotPos[i] - fragPos;
        float dist = length(lightVec);
        vec3 lightDir = normalize(lightVec);

        // Cone attenuation
        float theta = dot(lightDir, normalize(-uSpotDir[i]));
        float epsilon = uSpotCutOff[i] - uSpotOuterCutOff[i];
        float spotAtten = clamp((theta - uSpotOuterCutOff[i]) / epsilon, 0.0, 1.0);

        // Distance attenuation
        float distAtten = clamp(1.0 - dist / uSpotRange[i], 0.0, 1.0);
        distAtten *= distAtten;

        // Diffuse + specular
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);

        float spotShadow = CalcShadow(fragPosSpotSpace[i], uSpotShadowMap[i], -lightDir, normal);

        vec3 contribution = (1.0 - spotShadow) * (diff * texColor + spec * vec3(0.3))
                           * uSpotColor[i] * uSpotIntensity[i] * spotAtten * distAtten;
        spotResult += contribution;
    }

    FragColor = vec4(ambient + sunResult + spotResult, 1.0);
}
```

**Step 3: Commit**

```bash
git add resources/shaders/shadow.vs resources/shaders/shadow.fs resources/shaders/lit.vs resources/shaders/lit.fs
git commit -m "feat: add shadow depth shaders and Blinn-Phong lit shader with PCF shadows"
```

---

### Task 6: Create LightingSystem

**Files:**
- Create: `include/lighting/lighting_system.hpp`
- Create: `src/lighting/lighting_system.cpp`

**Step 1: Create header**

```cpp
#pragma once
#include "lighting/light.hpp"
#include "shaders/shader.hpp"
#include "glad.h"
#include <glm/glm.hpp>
#include <vector>
#include <functional>

class LightingSystem {
public:
    DirectionalLight sun;
    std::vector<SpotLight> spotLights;

    void Load();
    void Unload();

    void SetSun(const DirectionalLight& light);
    void AddSpotLight(const SpotLight& light);

    // Render shadow maps — calls drawScene for each shadow-casting light
    // drawScene receives the shadow shader program ID and light-space MVP uniform location
    void RenderShadowMaps(std::function<void(unsigned int shaderID, const glm::mat4& lightMVP)> drawScene);

    // Upload all light data + bind shadow maps to the given lit shader
    void ApplyToShader(unsigned int litShaderID, const glm::vec3& cameraPos);

    // Get shadow map texture unit offset (sun uses unit 1, spots use 2+)
    Shader& GetShadowShader() { return shadowShader; }

private:
    Shader shadowShader;

    // Sun shadow map
    unsigned int sunShadowFBO = 0;
    unsigned int sunShadowMap = 0;
    glm::mat4 sunLightSpaceMatrix;

    // Spot light shadow maps
    std::vector<unsigned int> spotShadowFBOs;
    std::vector<unsigned int> spotShadowMaps;
    std::vector<glm::mat4> spotLightSpaceMatrices;

    static const int SHADOW_WIDTH = 2048;
    static const int SHADOW_HEIGHT = 2048;

    void CreateShadowFBO(unsigned int& fbo, unsigned int& depthMap);
    glm::mat4 CalcSunLightSpaceMatrix();
    glm::mat4 CalcSpotLightSpaceMatrix(const SpotLight& light);
};
```

**Step 2: Create source**

```cpp
#include "lighting/lighting_system.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <iostream>

void LightingSystem::Load() {
    shadowShader = Shader::LoadShader("resources/shaders/shadow.vs", "resources/shaders/shadow.fs");

    // Create sun shadow map
    CreateShadowFBO(sunShadowFBO, sunShadowMap);
}

void LightingSystem::Unload() {
    shadowShader.Unload();

    glDeleteFramebuffers(1, &sunShadowFBO);
    glDeleteTextures(1, &sunShadowMap);
    sunShadowFBO = sunShadowMap = 0;

    for (auto fbo : spotShadowFBOs) glDeleteFramebuffers(1, &fbo);
    for (auto map : spotShadowMaps) glDeleteTextures(1, &map);
    spotShadowFBOs.clear();
    spotShadowMaps.clear();
    spotLightSpaceMatrices.clear();
    spotLights.clear();
}

void LightingSystem::SetSun(const DirectionalLight& light) {
    sun = light;
}

void LightingSystem::AddSpotLight(const SpotLight& light) {
    if ((int)spotLights.size() >= MAX_SPOT_LIGHTS) {
        std::cout << "WARNING::LIGHTING::MAX_SPOT_LIGHTS reached" << std::endl;
        return;
    }
    spotLights.push_back(light);

    unsigned int fbo, map;
    CreateShadowFBO(fbo, map);
    spotShadowFBOs.push_back(fbo);
    spotShadowMaps.push_back(map);
    spotLightSpaceMatrices.push_back(glm::mat4(1.0f));
}

void LightingSystem::CreateShadowFBO(unsigned int& fbo, unsigned int& depthMap) {
    glGenFramebuffers(1, &fbo);

    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 LightingSystem::CalcSunLightSpaceMatrix() {
    // Orthographic projection covering the scene
    float orthoSize = 80.0f;
    glm::mat4 lightProj = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 150.0f);

    glm::vec3 lightPos = -glm::normalize(sun.direction) * 60.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    return lightProj * lightView;
}

glm::mat4 LightingSystem::CalcSpotLightSpaceMatrix(const SpotLight& light) {
    float fov = glm::acos(light.outerCutOff) * 2.0f;
    glm::mat4 lightProj = glm::perspective(fov, 1.0f, 0.5f, light.range);

    glm::vec3 up = (glm::abs(light.direction.y) > 0.99f)
                   ? glm::vec3(0.0f, 0.0f, 1.0f)
                   : glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 lightView = glm::lookAt(light.position, light.position + light.direction, up);

    return lightProj * lightView;
}

void LightingSystem::RenderShadowMaps(
    std::function<void(unsigned int shaderID, const glm::mat4& lightMVP)> drawScene)
{
    glUseProgram(shadowShader.programID);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

    // Sun shadow pass
    sunLightSpaceMatrix = CalcSunLightSpaceMatrix();
    glBindFramebuffer(GL_FRAMEBUFFER, sunShadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    drawScene(shadowShader.programID, sunLightSpaceMatrix);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Spot light shadow passes
    for (int i = 0; i < (int)spotLights.size(); i++) {
        spotLightSpaceMatrices[i] = CalcSpotLightSpaceMatrix(spotLights[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, spotShadowFBOs[i]);
        glClear(GL_DEPTH_BUFFER_BIT);
        drawScene(shadowShader.programID, spotLightSpaceMatrices[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Restore viewport
    glViewport(0, 0, 800, 600);
}

void LightingSystem::ApplyToShader(unsigned int litShaderID, const glm::vec3& cameraPos) {
    glUseProgram(litShaderID);

    // Camera position
    glUniform3fv(glGetUniformLocation(litShaderID, "uViewPos"), 1, glm::value_ptr(cameraPos));

    // Ambient
    glm::vec3 ambient(0.15f, 0.15f, 0.2f);
    glUniform3fv(glGetUniformLocation(litShaderID, "uAmbientColor"), 1, glm::value_ptr(ambient));

    // Sun
    glUniform3fv(glGetUniformLocation(litShaderID, "uSunDirection"), 1, glm::value_ptr(sun.direction));
    glUniform3fv(glGetUniformLocation(litShaderID, "uSunColor"), 1, glm::value_ptr(sun.color));
    glUniform1f(glGetUniformLocation(litShaderID, "uSunIntensity"), sun.intensity);
    glUniformMatrix4fv(glGetUniformLocation(litShaderID, "uSunLightSpaceMVP"),
                       1, GL_FALSE, glm::value_ptr(sunLightSpaceMatrix));

    // Bind sun shadow map to texture unit 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sunShadowMap);
    glUniform1i(glGetUniformLocation(litShaderID, "uSunShadowMap"), 1);

    // Spot lights
    int numSpots = (int)spotLights.size();
    glUniform1i(glGetUniformLocation(litShaderID, "uNumSpotLights"), numSpots);

    for (int i = 0; i < numSpots; i++) {
        std::string idx = std::to_string(i);

        glUniform3fv(glGetUniformLocation(litShaderID, ("uSpotPos[" + idx + "]").c_str()),
                     1, glm::value_ptr(spotLights[i].position));
        glUniform3fv(glGetUniformLocation(litShaderID, ("uSpotDir[" + idx + "]").c_str()),
                     1, glm::value_ptr(spotLights[i].direction));
        glUniform3fv(glGetUniformLocation(litShaderID, ("uSpotColor[" + idx + "]").c_str()),
                     1, glm::value_ptr(spotLights[i].color));
        glUniform1f(glGetUniformLocation(litShaderID, ("uSpotIntensity[" + idx + "]").c_str()),
                    spotLights[i].intensity);
        glUniform1f(glGetUniformLocation(litShaderID, ("uSpotCutOff[" + idx + "]").c_str()),
                    spotLights[i].cutOff);
        glUniform1f(glGetUniformLocation(litShaderID, ("uSpotOuterCutOff[" + idx + "]").c_str()),
                    spotLights[i].outerCutOff);
        glUniform1f(glGetUniformLocation(litShaderID, ("uSpotRange[" + idx + "]").c_str()),
                    spotLights[i].range);

        glUniformMatrix4fv(glGetUniformLocation(litShaderID,
                           ("uSpotLightSpaceMVP[" + idx + "]").c_str()),
                           1, GL_FALSE, glm::value_ptr(spotLightSpaceMatrices[i]));

        // Bind spot shadow map to texture unit 2+i
        glActiveTexture(GL_TEXTURE2 + i);
        glBindTexture(GL_TEXTURE_2D, spotShadowMaps[i]);
        glUniform1i(glGetUniformLocation(litShaderID,
                    ("uSpotShadowMap[" + idx + "]").c_str()), 2 + i);
    }
}
```

**Step 3: Commit**

```bash
mkdir -p include/lighting src/lighting
git add include/lighting/light.hpp include/lighting/lighting_system.hpp src/lighting/lighting_system.cpp
git commit -m "feat: add LightingSystem with shadow mapping for sun and spot lights"
```

---

### Task 7: Integrate Lighting into Scene3D

**Files:**
- Modify: `include/scenes/scene3d.hpp`
- Modify: `src/scenes/scene3d.cpp`

**Step 1: Update Scene3D header**

Add lighting system include, lit shader, and a new `OnRenderGeometry` hook:

```cpp
#pragma once
#include "scenes/scene.hpp"
#include "scenes/skybox.hpp"
#include "scenes/terrain.hpp"
#include "scenes/terrain_generator.hpp"
#include "camera/camera.hpp"
#include "lighting/lighting_system.hpp"
#include <glm/glm.hpp>
#include <string>

struct Scene3DConfig {
    std::string name = "Scene";
    glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 3.0f);
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    bool useSkybox = true;
    std::string skyboxPath = "resources/textures/skybox/";

    bool useTerrain = true;
    TerrainGenerator* terrainGenerator = nullptr;

    bool useLighting = false;
};

class Scene3D : public Scene {
public:
    Scene3D(const Scene3DConfig& config);

protected:
    Camera camera;
    Skybox skybox;
    Terrain terrain;
    LightingSystem lighting;
    Shader litShader;
    Scene3DConfig config;
    bool cursorLocked = true;

    virtual void OnLoad() = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender(const glm::mat4& view, const glm::mat4& projection) = 0;
    virtual void OnUnload() = 0;

    // Override this to draw scene geometry for shadow passes
    // Called with shadow shader active — just set model matrix uniform and draw
    virtual void OnRenderGeometry(unsigned int shaderID, const glm::mat4& lightMVP) {}

private:
    void Load() override final;
    void Update() override final;
    void Render() override final;
    void Unload() override final;

    void RenderLit(const glm::mat4& view, const glm::mat4& projection);
    void RenderUnlit(const glm::mat4& view, const glm::mat4& projection);
};
```

**Step 2: Update Scene3D source**

```cpp
#include "scenes/scene3d.hpp"
#include "utils/time.hpp"
#include "glad.h"
#include "glfw3.h"
#include "imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Scene3D::Scene3D(const Scene3DConfig& cfg)
    : Scene(cfg.name), camera(cfg.cameraPos), config(cfg) {}

void Scene3D::Load() {
    glEnable(GL_DEPTH_TEST);

    GLFWwindow* window = glfwGetCurrentContext();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    cursorLocked = true;

    Time::Reset();

    if (config.useSkybox)
        skybox.Load();

    if (config.useTerrain) {
        if (config.terrainGenerator)
            terrain.Load(config.terrainGenerator);
        else
            terrain.Load();
    }

    if (config.useLighting) {
        lighting.Load();
        litShader = Shader::LoadShader("resources/shaders/lit.vs", "resources/shaders/lit.fs");
    }

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
        glm::radians(config.fov), 800.0f / 600.0f, config.nearPlane, config.farPlane);

    if (config.useSkybox)
        skybox.Render(view, projection);

    if (config.useLighting)
        RenderLit(view, projection);
    else
        RenderUnlit(view, projection);
}

void Scene3D::RenderUnlit(const glm::mat4& view, const glm::mat4& projection) {
    if (config.useTerrain)
        terrain.Render(view, projection);

    OnRender(view, projection);
}

void Scene3D::RenderLit(const glm::mat4& view, const glm::mat4& projection) {
    // 1. Shadow passes
    lighting.RenderShadowMaps([this](unsigned int shaderID, const glm::mat4& lightMVP) {
        // Draw terrain
        if (config.useTerrain) {
            int loc = glGetUniformLocation(shaderID, "uLightMVP");
            glm::mat4 model = glm::mat4(1.0f);
            glm::mat4 mvp = lightMVP * model;
            glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mvp));
            terrain.DrawGeometry();
        }

        // Let the scene draw its own geometry for shadows
        OnRenderGeometry(shaderID, lightMVP);
    });

    // 2. Main lit pass
    glUseProgram(litShader.programID);
    lighting.ApplyToShader(litShader.programID, camera.position);

    glUniformMatrix4fv(glGetUniformLocation(litShader.programID, "uView"),
                       1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(litShader.programID, "uProjection"),
                       1, GL_FALSE, glm::value_ptr(projection));

    // Draw terrain with lit shader
    if (config.useTerrain) {
        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(litShader.programID, "uModel"),
                           1, GL_FALSE, glm::value_ptr(model));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, terrain.GetTexture());
        glUniform1i(glGetUniformLocation(litShader.programID, "uTexture"), 0);

        terrain.DrawGeometry();
    }

    // Let scene render its lit objects
    OnRender(view, projection);
}

void Scene3D::Unload() {
    OnUnload();

    if (config.useSkybox)
        skybox.Unload();

    if (config.useTerrain)
        terrain.Unload();

    if (config.useLighting) {
        lighting.Unload();
        litShader.Unload();
    }

    glDisable(GL_DEPTH_TEST);
    glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    cursorLocked = true;
}
```

Note: `terrain.GetTexture()` needs to be added — a simple public getter for the texture ID.

**Step 3: Add GetTexture to Terrain**

In `include/scenes/terrain.hpp`, add to public section:
```cpp
    unsigned int GetTexture() const { return texture; }
```

**Step 4: Build and verify**

Run: `cmake --build build`
Test: P1 and P2 still work (useLighting = false, so RenderUnlit path is taken).

**Step 5: Commit**

```bash
git add include/scenes/scene3d.hpp src/scenes/scene3d.cpp include/scenes/terrain.hpp
git commit -m "feat: integrate LightingSystem into Scene3D with lit/unlit render paths"
```

---

### Task 8: Create P3 Scene (Lit Street with Streetlights)

**Files:**
- Modify: `include/scenes/p3_scene.hpp`
- Modify: `src/scenes/p3_scene.cpp`

**Step 1: Update P3Scene header**

```cpp
#pragma once
#include "scenes/scene3d.hpp"
#include "scenes/road.hpp"
#include "scenes/static_object.hpp"

class P3Scene : public Scene3D {
public:
    P3Scene();

    void OnLoad() override;
    void OnUpdate() override;
    void OnRender(const glm::mat4& view, const glm::mat4& projection) override;
    void OnUnload() override;
    void OnRenderGeometry(unsigned int shaderID, const glm::mat4& lightMVP) override;

private:
    Road road;
    StaticObjectRenderer objectRenderer;
    std::vector<ObjectInstance> objects;
    std::vector<unsigned int> loadedTextures;

    void SetupObjects();
    void SetupLights();

    // Helper: render all scene objects with a given MVP calculation
    void DrawSceneObjects(unsigned int shaderID, const glm::mat4& vpMatrix);
};
```

**Step 2: Update P3Scene source**

```cpp
#include "scenes/p3_scene.hpp"
#include "lighting/light.hpp"
#include "glad.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

P3Scene::P3Scene() : Scene3D({
    .name = "Lighting",
    .cameraPos = glm::vec3(0.0f, 15.0f, 50.0f),
    .farPlane = 200.0f,
    .useLighting = true
}) {}

void P3Scene::OnLoad() {
    road.Load();
    objectRenderer.Load();
    SetupObjects();
    SetupLights();
}

void P3Scene::SetupObjects() {
    unsigned int brickTex = objectRenderer.LoadTexture("resources/textures/objects/building.jpg");
    unsigned int woodTex  = objectRenderer.LoadTexture("resources/textures/objects/tree_trunk.jpg");
    loadedTextures.push_back(brickTex);
    loadedTextures.push_back(woodTex);

    // 5 buildings every 72 degrees, 8 units outside the road
    for (int i = 0; i < 5; i++) {
        float angle = glm::radians(i * 72.0f);
        float x = (40.0f + 8.0f) * cos(angle);
        float z = (30.0f + 8.0f) * sin(angle);
        float height = 6.0f + (rand() % 5);
        objects.push_back({ glm::vec3(x, 1.0f, z), glm::vec3(3, height, 3), brickTex });
    }

    // 5 trees staggered by 36 degrees, 4 units outside the road
    for (int i = 0; i < 5; i++) {
        float angle = glm::radians(i * 72.0f + 36.0f);
        float x = (40.0f + 4.0f) * cos(angle);
        float z = (30.0f + 4.0f) * sin(angle);
        float height = 6.0f + (rand() % 5);
        objects.push_back({ glm::vec3(x, 1.0f, z), glm::vec3(1, height, 1), woodTex });
    }
}

void P3Scene::SetupLights() {
    // Sun — warm directional light from upper-left
    DirectionalLight sun;
    sun.direction = glm::vec3(-0.5f, -1.0f, -0.3f);
    sun.color = glm::vec3(1.0f, 0.95f, 0.8f);
    sun.intensity = 0.8f;
    lighting.SetSun(sun);

    // 4 streetlights around the circuit
    float streetlightAngles[] = { 0.0f, 90.0f, 180.0f, 270.0f };
    for (int i = 0; i < 4; i++) {
        float angle = glm::radians(streetlightAngles[i]);
        float x = 37.5f * cos(angle);  // middle of road
        float z = 27.5f * sin(angle);

        SpotLight spot;
        spot.position = glm::vec3(x, 12.0f, z);    // 12 units high
        spot.direction = glm::vec3(0.0f, -1.0f, 0.0f); // pointing straight down
        spot.color = glm::vec3(1.0f, 0.9f, 0.7f);  // warm white
        spot.intensity = 2.0f;
        spot.cutOff = glm::cos(glm::radians(30.0f));
        spot.outerCutOff = glm::cos(glm::radians(40.0f));
        spot.range = 30.0f;
        lighting.AddSpotLight(spot);
    }
}

void P3Scene::OnRenderGeometry(unsigned int shaderID, const glm::mat4& lightMVP) {
    int loc = glGetUniformLocation(shaderID, "uLightMVP");

    // Road
    glm::mat4 roadMVP = lightMVP * glm::mat4(1.0f);
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(roadMVP));
    road.DrawGeometry();

    // Objects
    for (const auto& obj : objects) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, obj.position);
        model = glm::scale(model, obj.scale);
        model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f));
        glm::mat4 mvp = lightMVP * model;
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mvp));
        objectRenderer.BindAndDraw();
    }
}

void P3Scene::OnUpdate() {
}

void P3Scene::OnRender(const glm::mat4& view, const glm::mat4& projection) {
    if (config.useLighting) {
        // Lit rendering — litShader is already active from Scene3D::RenderLit
        // Road
        glm::mat4 roadModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(litShader.programID, "uModel"),
                           1, GL_FALSE, glm::value_ptr(roadModel));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, road.GetTexture());
        glUniform1i(glGetUniformLocation(litShader.programID, "uTexture"), 0);
        road.DrawGeometry();

        // Objects
        for (const auto& obj : objects) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            model = glm::scale(model, obj.scale);
            model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f));
            glUniformMatrix4fv(glGetUniformLocation(litShader.programID, "uModel"),
                               1, GL_FALSE, glm::value_ptr(model));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, obj.textureID);
            glUniform1i(glGetUniformLocation(litShader.programID, "uTexture"), 0);
            objectRenderer.BindAndDraw();
        }
    } else {
        road.Render(view, projection);
        objectRenderer.Render(objects, view, projection);
    }
}

void P3Scene::OnUnload() {
    road.Unload();
    objectRenderer.Unload();
    for (auto tex : loadedTextures) glDeleteTextures(1, &tex);
    loadedTextures.clear();
    objects.clear();
}
```

Note: `road.GetTexture()` needs adding — same pattern as terrain.

**Step 3: Add GetTexture to Road**

In `include/scenes/road.hpp`, add to public section:
```cpp
    unsigned int GetTexture() const { return texture; }
    void DrawGeometry();
```

**Step 4: Build and verify**

Run: `cmake --build build`
Test: Switch to "Lighting" tab. Should see:
- Lit terrain, road, and objects with sun shadows
- 4 spot lights illuminating sections of the road with their own shadows
- Q/E camera roll works

**Step 5: Commit**

```bash
git add include/scenes/p3_scene.hpp src/scenes/p3_scene.cpp include/scenes/road.hpp
git commit -m "feat: P3 scene with sun + 4 streetlight spot shadows, reuses P2 assets"
```

---

## Verification Checklist

- [ ] Camera Q/E roll works in all scenes
- [ ] P1 and P2 scenes render identically (unlit path, unchanged behavior)
- [ ] P3 scene has directional sun casting shadows on terrain, road, objects
- [ ] P3 scene has 4 spot streetlights casting individual shadows
- [ ] Shadow acne is minimal (bias works)
- [ ] Switching tabs works (ImGui fix still active)
- [ ] Scene3DConfig allows future scenes with no terrain / different skybox
- [ ] `Time::deltaTime` still accessible
