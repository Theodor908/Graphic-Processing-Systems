# P4 — Drivable Car with AABB Collision Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add a WASD-controlled car to P4 that collides with buildings and poles in the P3 lit environment.

**Architecture:** P4Scene extends Scene3D (like P3) with the same objects/lights. A car struct holds position + yaw. Each frame: process WASD input for car movement, resolve collisions per-axis against static AABBs, overwrite camera to follow behind car. Visual collision flash via a decaying timer.

**Tech Stack:** OpenGL 3.3, GLM, GLFW, ImGui, existing Scene3D/LightingSystem/StaticObjectRenderer

---

### Task 1: Create AABB collision header

**Files:**
- Create: `include/collision/aabb.hpp`

**Step 1: Write the AABB header**

```cpp
#pragma once
#include <glm/glm.hpp>
#include "scenes/static_object.hpp"

struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    bool Overlaps(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }
};

// Build AABB from ObjectInstance (matches the render transform:
//   translate(position) * scale(scale) * translate(0, 0.5, 0))
// Result: x in [pos.x - scale.x/2, pos.x + scale.x/2]
//         y in [pos.y, pos.y + scale.y]
//         z in [pos.z - scale.z/2, pos.z + scale.z/2]
inline AABB AABBFromObject(const ObjectInstance& obj) {
    glm::vec3 half(obj.scale.x * 0.5f, 0.0f, obj.scale.z * 0.5f);
    return {
        glm::vec3(obj.position.x - half.x, obj.position.y, obj.position.z - half.z),
        glm::vec3(obj.position.x + half.x, obj.position.y + obj.scale.y, obj.position.z + half.z)
    };
}

// Build AABB for a car-like object at position with fixed footprint
// Car is centered at position.xz, base at position.y
inline AABB AABBFromCar(const glm::vec3& position, const glm::vec3& halfExtents) {
    return {
        position - halfExtents,
        position + halfExtents
    };
}
```

**Step 2: Verify it compiles**

Run: `export PATH="/c/msys64/ucrt64/bin:$PATH" && cmake --build build 2>&1`

(Header-only, won't compile yet until included somewhere — just verify no syntax issues by including in next task.)

---

### Task 2: Write P4Scene header

**Files:**
- Modify: `include/scenes/p4_scene.hpp` (replace contents)

**Step 1: Rewrite P4Scene to extend Scene3D**

Replace the entire file:

```cpp
#pragma once
#include "scenes/scene3d.hpp"
#include "scenes/road.hpp"
#include "scenes/static_object.hpp"
#include "collision/aabb.hpp"

class P4Scene : public Scene3D {
public:
    P4Scene();

    void OnLoad() override;
    void OnUpdate() override;
    void OnRender(const glm::mat4& view, const glm::mat4& projection) override;
    void OnUnload() override;
    void OnRenderGeometry(unsigned int shaderID, const glm::mat4& lightMVP) override;

private:
    Road road;
    StaticObjectRenderer objectRenderer;
    std::vector<ObjectInstance> objects;
    std::vector<AABB> colliders;  // precomputed AABBs for static objects
    std::vector<unsigned int> loadedTextures;

    // Car state
    glm::vec3 carPos = glm::vec3(37.5f, 0.0f, 0.0f);  // start on the road
    float carYaw = 0.0f;        // degrees, rotation around Y
    float carSpeed = 0.0f;
    float collisionTimer = 0.0f; // >0 means flash red

    static constexpr float CAR_MAX_SPEED = 15.0f;
    static constexpr float CAR_ACCEL = 20.0f;
    static constexpr float CAR_BRAKE = 30.0f;
    static constexpr float CAR_FRICTION = 8.0f;
    static constexpr float CAR_TURN_SPEED = 120.0f;
    static constexpr glm::vec3 CAR_SCALE = glm::vec3(1.5f, 1.0f, 2.5f);
    static constexpr glm::vec3 CAR_HALF = glm::vec3(1.0f, 0.5f, 1.0f); // conservative AABB

    // Camera follow
    static constexpr float CAM_DISTANCE = 12.0f;
    static constexpr float CAM_HEIGHT = 6.0f;

    void SetupObjects();
    void SetupLights();
    void UpdateCar(float dt);
    void RenderCar(unsigned int shaderID);
    glm::mat4 GetCarModelMatrix() const;
};
```

**Step 2: Build to check header parses**

Run: `export PATH="/c/msys64/ucrt64/bin:$PATH" && cmake -S . -B build -G Ninja && cmake --build build 2>&1`

Expected: may fail on missing implementations — that's fine, move to next task.

---

### Task 3: Implement P4Scene

**Files:**
- Modify: `src/scenes/p4_scene.cpp` (replace contents)

**Step 1: Write the full P4Scene implementation**

```cpp
#include "scenes/p4_scene.hpp"
#include "lighting/light.hpp"
#include "utils/time.hpp"
#include "glad.h"
#include "glfw3.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

P4Scene::P4Scene() : Scene3D({
    .name = "Collisions",
    .cameraPos = glm::vec3(0.0f, 15.0f, 50.0f),
    .farPlane = 200.0f,
    .useLighting = true
}) {}

void P4Scene::OnLoad() {
    road.Load();
    objectRenderer.Load();
    SetupObjects();
    SetupLights();

    // Precompute AABBs for all static objects
    colliders.reserve(objects.size());
    for (const auto& obj : objects)
        colliders.push_back(AABBFromObject(obj));
}

void P4Scene::SetupObjects() {
    // Identical to P3 — buildings, trees, lamp poles
    unsigned int brickTex = objectRenderer.LoadTexture("resources/textures/objects/building.jpg");
    unsigned int woodTex  = objectRenderer.LoadTexture("resources/textures/objects/tree_trunk.jpg");
    unsigned int steelTex = objectRenderer.LoadTexture("resources/textures/objects/steel.jpg");
    loadedTextures.push_back(brickTex);
    loadedTextures.push_back(woodTex);
    loadedTextures.push_back(steelTex);

    // 5 buildings
    for (int i = 0; i < 5; i++) {
        float angle = glm::radians(i * 72.0f);
        float x = (40.0f + 8.0f) * std::cos(angle);
        float z = (30.0f + 8.0f) * std::sin(angle);
        float height = 6.0f + (rand() % 5);
        objects.push_back({ glm::vec3(x, 1.0f, z), glm::vec3(3, height, 3), brickTex });
    }

    // 5 trees
    for (int i = 0; i < 5; i++) {
        float angle = glm::radians(i * 72.0f + 36.0f);
        float x = (40.0f + 4.0f) * std::cos(angle);
        float z = (30.0f + 4.0f) * std::sin(angle);
        float height = 6.0f + (rand() % 5);
        objects.push_back({ glm::vec3(x, 1.0f, z), glm::vec3(1, height, 1), woodTex });
    }

    // 4 lamp poles
    float streetlightAngles[] = { 0.0f, 90.0f, 180.0f, 270.0f };
    for (int i = 0; i < 4; i++) {
        float angle = glm::radians(streetlightAngles[i]);
        float lightX = 37.5f * std::cos(angle);
        float lightZ = 27.5f * std::sin(angle);
        float poleX = 40.0f * std::cos(angle);
        float poleZ = 30.0f * std::sin(angle);
        float poleHeight = 12.0f;
        float armThickness = 0.3f;

        objects.push_back({ glm::vec3(poleX, 0.0f, poleZ),
                            glm::vec3(armThickness, poleHeight, armThickness), steelTex });

        float dx = lightX - poleX;
        float dz = lightZ - poleZ;
        float armLen = std::sqrt(dx * dx + dz * dz);
        float armX = (poleX + lightX) * 0.5f;
        float armZ = (poleZ + lightZ) * 0.5f;
        glm::vec3 armScale = (std::abs(dx) > std::abs(dz))
            ? glm::vec3(armLen, armThickness, armThickness)
            : glm::vec3(armThickness, armThickness, armLen);

        objects.push_back({ glm::vec3(armX, poleHeight - armThickness, armZ),
                            armScale, steelTex });
    }
}

void P4Scene::SetupLights() {
    // Identical to P3
    DirectionalLight sun;
    sun.direction = glm::vec3(-0.5f, -1.0f, -0.3f);
    sun.color = glm::vec3(1.0f, 0.95f, 0.8f);
    sun.intensity = 0.8f;
    lighting.SetSun(sun);

    float streetlightAngles[] = { 0.0f, 90.0f, 180.0f, 270.0f };
    for (int i = 0; i < 4; i++) {
        float angle = glm::radians(streetlightAngles[i]);
        SpotLight spot;
        spot.position = glm::vec3(37.5f * std::cos(angle), 12.0f, 27.5f * std::sin(angle));
        spot.direction = glm::vec3(0.0f, -1.0f, 0.0f);
        spot.color = glm::vec3(1.0f, 0.9f, 0.7f);
        spot.intensity = 2.0f;
        spot.cutOff = glm::cos(glm::radians(30.0f));
        spot.outerCutOff = glm::cos(glm::radians(40.0f));
        spot.range = 30.0f;
        lighting.AddSpotLight(spot);
    }

    glm::vec3 pointColors[] = {
        glm::vec3(1.0f, 0.6f, 0.3f),
        glm::vec3(0.3f, 0.6f, 1.0f),
        glm::vec3(0.5f, 1.0f, 0.5f)
    };
    float pointAngles[] = { 60.0f, 180.0f, 300.0f };
    for (int i = 0; i < 3; i++) {
        float angle = glm::radians(pointAngles[i]);
        PointLight pt;
        pt.position = glm::vec3(25.0f * std::cos(angle), 5.0f, 25.0f * std::sin(angle));
        pt.color = pointColors[i];
        pt.intensity = 3.0f;
        pt.constant = 1.0f;
        pt.linear = 0.09f;
        pt.quadratic = 0.032f;
        lighting.AddPointLight(pt);
    }
}

glm::mat4 P4Scene::GetCarModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, carPos);
    model = glm::rotate(model, glm::radians(carYaw), glm::vec3(0, 1, 0));
    model = glm::scale(model, CAR_SCALE);
    model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f)); // base on ground
    return model;
}

void P4Scene::UpdateCar(float dt) {
    GLFWwindow* window = glfwGetCurrentContext();

    // Turning (only when moving)
    if (std::abs(carSpeed) > 0.5f) {
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            carYaw += CAR_TURN_SPEED * dt;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            carYaw -= CAR_TURN_SPEED * dt;
    }

    // Acceleration / braking
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        carSpeed += CAR_ACCEL * dt;
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        carSpeed -= CAR_BRAKE * dt;
    else {
        // Friction
        if (carSpeed > 0.0f) carSpeed = glm::max(0.0f, carSpeed - CAR_FRICTION * dt);
        else if (carSpeed < 0.0f) carSpeed = glm::min(0.0f, carSpeed + CAR_FRICTION * dt);
    }
    carSpeed = glm::clamp(carSpeed, -CAR_MAX_SPEED * 0.5f, CAR_MAX_SPEED);

    // Forward direction from yaw
    float rad = glm::radians(carYaw);
    glm::vec3 forward(std::sin(rad), 0.0f, std::cos(rad));
    glm::vec3 movement = forward * carSpeed * dt;

    // Per-axis collision resolution
    glm::vec3 newPos = carPos;

    // Try X
    newPos.x += movement.x;
    AABB carBox = AABBFromCar(newPos, CAR_HALF);
    bool hitX = false;
    for (const auto& col : colliders) {
        if (carBox.Overlaps(col)) { hitX = true; break; }
    }
    if (hitX) {
        newPos.x = carPos.x; // revert X
        collisionTimer = 0.3f;
    }

    // Try Z
    newPos.z += movement.z;
    carBox = AABBFromCar(newPos, CAR_HALF);
    bool hitZ = false;
    for (const auto& col : colliders) {
        if (carBox.Overlaps(col)) { hitZ = true; break; }
    }
    if (hitZ) {
        newPos.z = carPos.z; // revert Z
        collisionTimer = 0.3f;
    }

    // Kill speed on head-on collision
    if (hitX && hitZ) carSpeed = 0.0f;

    carPos = newPos;

    // Decay collision flash
    if (collisionTimer > 0.0f)
        collisionTimer -= dt;

    // Camera follows behind car
    camera.position = carPos
        + glm::vec3(-std::sin(rad) * CAM_DISTANCE, CAM_HEIGHT, -std::cos(rad) * CAM_DISTANCE);
    camera.direction = glm::normalize(carPos + glm::vec3(0, 1, 0) - camera.position);
}

void P4Scene::OnUpdate() {
    UpdateCar(Time::deltaTime);
}

void P4Scene::RenderCar(unsigned int shaderID) {
    glm::mat4 model = GetCarModelMatrix();
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "uModel"),
                       1, GL_FALSE, glm::value_ptr(model));

    // Use steel texture for the car (reuse loadedTextures[2])
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, loadedTextures[2]); // steelTex
    glUniform1i(glGetUniformLocation(shaderID, "uTexture"), 0);
    objectRenderer.BindAndDraw();
}

void P4Scene::OnRenderGeometry(unsigned int shaderID, const glm::mat4& lightMVP) {
    int loc = glGetUniformLocation(shaderID, "uLightMVP");

    // Road
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(lightMVP));
    road.DrawGeometry();

    // Static objects
    for (const auto& obj : objects) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, obj.position);
        model = glm::scale(model, obj.scale);
        model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f));
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(lightMVP * model));
        objectRenderer.BindAndDraw();
    }

    // Car shadow
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(lightMVP * GetCarModelMatrix()));
    objectRenderer.BindAndDraw();
}

void P4Scene::OnRender(const glm::mat4& view, const glm::mat4& projection) {
    if (config.useLighting) {
        // Road
        glm::mat4 roadModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(litShader.programID, "uModel"),
                           1, GL_FALSE, glm::value_ptr(roadModel));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, road.GetTexture());
        glUniform1i(glGetUniformLocation(litShader.programID, "uTexture"), 0);
        road.DrawGeometry();

        // Static objects
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

        // Car
        RenderCar(litShader.programID);
    } else {
        road.Render(view, projection);
        objectRenderer.Render(objects, view, projection);
    }

    // Collision flash indicator
    if (collisionTimer > 0.0f) {
        ImGui::Begin("##collision", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
        ImGui::SetWindowPos(ImVec2(10, 10));
        ImGui::TextColored(ImVec4(1, 0, 0, collisionTimer / 0.3f), "COLLISION!");
        ImGui::End();
    }

    // Car speed HUD
    ImGui::Begin("Car");
    ImGui::Text("Speed: %.1f", carSpeed);
    ImGui::Text("Position: (%.1f, %.1f)", carPos.x, carPos.z);
    ImGui::Text("Yaw: %.0f", carYaw);
    ImGui::End();
}

void P4Scene::OnUnload() {
    road.Unload();
    objectRenderer.Unload();
    for (auto tex : loadedTextures) glDeleteTextures(1, &tex);
    loadedTextures.clear();
    objects.clear();
    colliders.clear();
}
```

**Step 2: Build and fix any errors**

Run: `export PATH="/c/msys64/ucrt64/bin:$PATH" && cmake -S . -B build -G Ninja && cmake --build build 2>&1`

Expected: clean build.

---

### Task 4: Handle camera input conflict

**Context:** Scene3D::Update() calls `camera.Update()` when `cursorLocked` is true, which processes WASD for camera movement. P4 uses WASD for the car and overwrites camera position in `OnUpdate()`. The camera's WASD movement is effectively discarded each frame. However, `camera.Update()` also processes mouse (recenters cursor, modifies yaw/pitch), which is harmless since we overwrite `camera.direction` too.

**No code change needed** — the current approach of overwriting camera state in `OnUpdate()` works correctly. The camera's intermediate WASD/mouse state is thrown away each frame.

Verify by running the app, switching to P4, and confirming:
- WASD drives the car (not the camera)
- Camera follows behind the car
- No jittery camera behavior

---

### Task 5: Build and test

**Step 1: Full rebuild**

Run: `export PATH="/c/msys64/ucrt64/bin:$PATH" && cmake -S . -B build -G Ninja && cmake --build build 2>&1`

Expected: clean build, 0 errors.

**Step 2: Manual verification checklist**

Run the app and switch to P4 tab:
- [ ] Scene loads with buildings, trees, lamp poles, road, lighting
- [ ] Car cube visible on the road
- [ ] W/S moves car forward/backward
- [ ] A/D turns car left/right
- [ ] Camera follows behind car smoothly
- [ ] Driving into a building stops the car
- [ ] Driving into a lamp pole stops the car
- [ ] Red "COLLISION!" text flashes on hit
- [ ] Wall-sliding works (approach building at angle, slide along it)
- [ ] Car casts shadows
- [ ] ImGui "Car" panel shows speed/position/yaw
