# P5 — AI Cars + Wandering Cubes Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add 2 AI cars orbiting the road and 5 wandering cubes bouncing off buildings to a fully lit scene with the player-controlled car from P4.

**Architecture:** P5Scene extends Scene3D like P4, reuses the same environment/lights. AI cars advance an angle parameter over time and derive position/yaw from the elliptical road. Wandering cubes move in random XZ directions and reflect off static colliders. Player car collides with everything (static + dynamic).

**Tech Stack:** OpenGL 3.3, GLM, GLFW, ImGui, existing Scene3D/AABB/LightingSystem

---

### Task 1: Write P5Scene header

**Files:**
- Modify: `include/scenes/p5_scene.hpp`

**Step 1: Replace contents with Scene3D subclass**

```cpp
#pragma once
#include "scenes/scene3d.hpp"
#include "scenes/road.hpp"
#include "scenes/static_object.hpp"
#include "collision/aabb.hpp"
#include <vector>

struct AICar {
    float angle;       // current position on ellipse (radians)
    float speed;       // angular speed (radians/sec)
    glm::vec3 pos;
    float yaw;
};

struct WanderCube {
    glm::vec3 pos;
    glm::vec3 dir;     // normalized XZ direction
    float speed;
};

class P5Scene : public Scene3D {
public:
    P5Scene();

    void OnLoad() override;
    void OnUpdate() override;
    void OnRender(const glm::mat4& view, const glm::mat4& projection) override;
    void OnUnload() override;
    void OnRenderGeometry(unsigned int shaderID, const glm::mat4& lightMVP) override;

private:
    Road road;
    StaticObjectRenderer objectRenderer;
    std::vector<ObjectInstance> objects;
    std::vector<AABB> staticColliders;
    std::vector<unsigned int> loadedTextures;

    // Player car (same as P4)
    glm::vec3 carPos = glm::vec3(37.5f, 0.0f, 0.0f);
    float carYaw = 0.0f;
    float carSpeed = 0.0f;
    float collisionTimer = 0.0f;

    static constexpr float CAR_MAX_SPEED = 15.0f;
    static constexpr float CAR_ACCEL = 20.0f;
    static constexpr float CAR_BRAKE = 30.0f;
    static constexpr float CAR_FRICTION = 8.0f;
    static constexpr float CAR_TURN_SPEED = 120.0f;
    static constexpr float CAM_DISTANCE = 12.0f;
    static constexpr float CAM_HEIGHT = 6.0f;
    glm::vec3 carScale = glm::vec3(1.5f, 1.0f, 2.5f);
    glm::vec3 carHalf = glm::vec3(1.0f, 1.5f, 1.0f);

    // AI cars
    std::vector<AICar> aiCars;
    glm::vec3 aiCarScale = glm::vec3(1.5f, 1.0f, 2.5f);
    glm::vec3 aiCarHalf = glm::vec3(1.0f, 1.5f, 1.0f);

    // Wandering cubes
    std::vector<WanderCube> wanderCubes;
    glm::vec3 wanderScale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 wanderHalf = glm::vec3(0.5f, 1.5f, 0.5f);

    // Road ellipse center-line radii
    static constexpr float ROAD_RX = 37.5f;
    static constexpr float ROAD_RZ = 27.5f;

    void SetupObjects();
    void SetupLights();
    void UpdatePlayerCar(float dt);
    void UpdateAICars(float dt);
    void UpdateWanderCubes(float dt);
    void CollectDynamicColliders(std::vector<AABB>& out) const;
    glm::mat4 GetPlayerCarModel() const;
    glm::mat4 GetAICarModel(const AICar& ai) const;
    glm::mat4 GetWanderCubeModel(const WanderCube& wc) const;
    void RenderDynamic(unsigned int shaderID);
};
```

---

### Task 2: Implement P5Scene

**Files:**
- Modify: `src/scenes/p5_scene.cpp`

**Step 1: Replace contents with full implementation**

```cpp
#include "scenes/p5_scene.hpp"
#include "lighting/light.hpp"
#include "utils/time.hpp"
#include "glad.h"
#include "glfw3.h"
#include "imgui.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdlib>

P5Scene::P5Scene() : Scene3D({
    .name = "Scene 5",
    .cameraPos = glm::vec3(0.0f, 15.0f, 50.0f),
    .farPlane = 200.0f,
    .useLighting = true
}) {}

void P5Scene::OnLoad() {
    road.Load();
    objectRenderer.Load();
    SetupObjects();
    SetupLights();

    // Static colliders
    staticColliders.reserve(objects.size());
    for (const auto& obj : objects)
        staticColliders.push_back(AABBFromObject(obj));

    // 2 AI cars at opposite sides of the ellipse
    aiCars.push_back({ 0.0f, 0.8f, glm::vec3(0), 0.0f });
    aiCars.push_back({ glm::radians(180.0f), 1.1f, glm::vec3(0), 0.0f });

    // 5 wandering cubes in random positions outside the road
    srand(42); // deterministic for reproducibility
    for (int i = 0; i < 5; i++) {
        float angle = glm::radians(i * 72.0f + 15.0f);
        float radius = 50.0f + (rand() % 10);
        glm::vec3 pos(radius * std::cos(angle), 0.0f, radius * std::sin(angle));

        float dirAngle = glm::radians((float)(rand() % 360));
        glm::vec3 dir(std::cos(dirAngle), 0.0f, std::sin(dirAngle));

        wanderCubes.push_back({ pos, dir, 3.0f + (rand() % 3) });
    }
}

// SetupObjects and SetupLights are identical to P4
void P5Scene::SetupObjects() {
    unsigned int brickTex = objectRenderer.LoadTexture("resources/textures/objects/building.jpg");
    unsigned int woodTex  = objectRenderer.LoadTexture("resources/textures/objects/tree_trunk.jpg");
    unsigned int steelTex = objectRenderer.LoadTexture("resources/textures/objects/steel.jpg");
    loadedTextures.push_back(brickTex);
    loadedTextures.push_back(woodTex);
    loadedTextures.push_back(steelTex);

    for (int i = 0; i < 5; i++) {
        float angle = glm::radians(i * 72.0f);
        float x = 48.0f * std::cos(angle);
        float z = 38.0f * std::sin(angle);
        float height = 6.0f + (rand() % 5);
        objects.push_back({ glm::vec3(x, 1.0f, z), glm::vec3(3, height, 3), brickTex });
    }
    for (int i = 0; i < 5; i++) {
        float angle = glm::radians(i * 72.0f + 36.0f);
        float x = 44.0f * std::cos(angle);
        float z = 34.0f * std::sin(angle);
        float height = 6.0f + (rand() % 5);
        objects.push_back({ glm::vec3(x, 1.0f, z), glm::vec3(1, height, 1), woodTex });
    }
    float streetlightAngles[] = { 0.0f, 90.0f, 180.0f, 270.0f };
    for (int i = 0; i < 4; i++) {
        float angle = glm::radians(streetlightAngles[i]);
        float lightX = ROAD_RX * std::cos(angle);
        float lightZ = ROAD_RZ * std::sin(angle);
        float poleX = 40.0f * std::cos(angle);
        float poleZ = 30.0f * std::sin(angle);
        float poleHeight = 12.0f, armThickness = 0.3f;

        objects.push_back({ glm::vec3(poleX, 0.0f, poleZ),
                            glm::vec3(armThickness, poleHeight, armThickness), steelTex });

        float dx = lightX - poleX, dz = lightZ - poleZ;
        float armLen = std::sqrt(dx*dx + dz*dz);
        glm::vec3 armScale = (std::abs(dx) > std::abs(dz))
            ? glm::vec3(armLen, armThickness, armThickness)
            : glm::vec3(armThickness, armThickness, armLen);
        objects.push_back({ glm::vec3((poleX+lightX)*0.5f, poleHeight-armThickness, (poleZ+lightZ)*0.5f),
                            armScale, steelTex });
    }
}

void P5Scene::SetupLights() {
    DirectionalLight sun;
    sun.direction = glm::vec3(-0.5f, -1.0f, -0.3f);
    sun.color = glm::vec3(1.0f, 0.95f, 0.8f);
    sun.intensity = 0.8f;
    lighting.SetSun(sun);

    float slAngles[] = { 0.0f, 90.0f, 180.0f, 270.0f };
    for (int i = 0; i < 4; i++) {
        float a = glm::radians(slAngles[i]);
        SpotLight s;
        s.position = glm::vec3(ROAD_RX*std::cos(a), 12.0f, ROAD_RZ*std::sin(a));
        s.direction = glm::vec3(0,-1,0);
        s.color = glm::vec3(1,0.9f,0.7f);
        s.intensity = 2.0f;
        s.cutOff = glm::cos(glm::radians(30.0f));
        s.outerCutOff = glm::cos(glm::radians(40.0f));
        s.range = 30.0f;
        lighting.AddSpotLight(s);
    }

    glm::vec3 pc[] = { {1,0.6f,0.3f}, {0.3f,0.6f,1}, {0.5f,1,0.5f} };
    float pa[] = { 60.0f, 180.0f, 300.0f };
    for (int i = 0; i < 3; i++) {
        float a = glm::radians(pa[i]);
        PointLight p;
        p.position = glm::vec3(25*std::cos(a), 5, 25*std::sin(a));
        p.color = pc[i]; p.intensity = 3; p.constant = 1;
        p.linear = 0.09f; p.quadratic = 0.032f;
        lighting.AddPointLight(p);
    }
}

// --- Model matrices ---

glm::mat4 P5Scene::GetPlayerCarModel() const {
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, carPos);
    m = glm::rotate(m, glm::radians(carYaw), glm::vec3(0,1,0));
    m = glm::scale(m, carScale);
    m = glm::translate(m, glm::vec3(0, 1, 0));
    return m;
}

glm::mat4 P5Scene::GetAICarModel(const AICar& ai) const {
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, ai.pos);
    m = glm::rotate(m, glm::radians(ai.yaw), glm::vec3(0,1,0));
    m = glm::scale(m, aiCarScale);
    m = glm::translate(m, glm::vec3(0, 1, 0));
    return m;
}

glm::mat4 P5Scene::GetWanderCubeModel(const WanderCube& wc) const {
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, wc.pos);
    m = glm::scale(m, wanderScale);
    m = glm::translate(m, glm::vec3(0, 0.5f, 0));
    return m;
}

// --- Update logic ---

void P5Scene::CollectDynamicColliders(std::vector<AABB>& out) const {
    for (const auto& ai : aiCars)
        out.push_back(AABBFromCar(ai.pos, aiCarHalf));
    for (const auto& wc : wanderCubes)
        out.push_back(AABBFromCar(wc.pos, wanderHalf));
}

void P5Scene::UpdateAICars(float dt) {
    for (auto& ai : aiCars) {
        ai.angle += ai.speed * dt;
        if (ai.angle > 2.0f * 3.14159265f) ai.angle -= 2.0f * 3.14159265f;

        ai.pos = glm::vec3(
            ROAD_RX * std::cos(ai.angle),
            0.0f,
            ROAD_RZ * std::sin(ai.angle)
        );

        // Yaw from ellipse tangent: d/dt(cos(t),sin(t)) = (-sin(t),cos(t))
        float tx = -ROAD_RX * std::sin(ai.angle);
        float tz =  ROAD_RZ * std::cos(ai.angle);
        ai.yaw = glm::degrees(std::atan2(tx, tz));
    }
}

void P5Scene::UpdateWanderCubes(float dt) {
    for (auto& wc : wanderCubes) {
        glm::vec3 movement = wc.dir * wc.speed * dt;
        glm::vec3 newPos = wc.pos;

        // Try X
        newPos.x += movement.x;
        AABB box = AABBFromCar(newPos, wanderHalf);
        bool hitX = false;
        for (const auto& col : staticColliders) {
            if (box.Overlaps(col)) { hitX = true; break; }
        }
        if (hitX) {
            newPos.x = wc.pos.x;
            wc.dir.x = -wc.dir.x; // reflect X
        }

        // Try Z
        newPos.z += movement.z;
        box = AABBFromCar(newPos, wanderHalf);
        bool hitZ = false;
        for (const auto& col : staticColliders) {
            if (box.Overlaps(col)) { hitZ = true; break; }
        }
        if (hitZ) {
            newPos.z = wc.pos.z;
            wc.dir.z = -wc.dir.z; // reflect Z
        }

        wc.pos = newPos;
    }
}

void P5Scene::UpdatePlayerCar(float dt) {
    GLFWwindow* window = glfwGetCurrentContext();

    if (std::abs(carSpeed) > 0.5f) {
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) carYaw += CAR_TURN_SPEED * dt;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) carYaw -= CAR_TURN_SPEED * dt;
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) carSpeed += CAR_ACCEL * dt;
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) carSpeed -= CAR_BRAKE * dt;
    else {
        if (carSpeed > 0) carSpeed = glm::max(0.0f, carSpeed - CAR_FRICTION * dt);
        else if (carSpeed < 0) carSpeed = glm::min(0.0f, carSpeed + CAR_FRICTION * dt);
    }
    carSpeed = glm::clamp(carSpeed, -CAR_MAX_SPEED * 0.5f, CAR_MAX_SPEED);

    float rad = glm::radians(carYaw);
    glm::vec3 forward(std::sin(rad), 0, std::cos(rad));
    glm::vec3 movement = forward * carSpeed * dt;

    // Collect ALL colliders: static + dynamic
    std::vector<AABB> allColliders = staticColliders;
    CollectDynamicColliders(allColliders);

    glm::vec3 newPos = carPos;

    newPos.x += movement.x;
    AABB carBox = AABBFromCar(newPos, carHalf);
    bool hitX = false;
    for (const auto& col : allColliders) {
        if (carBox.Overlaps(col)) { hitX = true; break; }
    }
    if (hitX) { newPos.x = carPos.x; collisionTimer = 0.3f; }

    newPos.z += movement.z;
    carBox = AABBFromCar(newPos, carHalf);
    bool hitZ = false;
    for (const auto& col : allColliders) {
        if (carBox.Overlaps(col)) { hitZ = true; break; }
    }
    if (hitZ) { newPos.z = carPos.z; collisionTimer = 0.3f; }

    if (hitX && hitZ) carSpeed = 0.0f;
    carPos = newPos;

    if (collisionTimer > 0) collisionTimer -= dt;

    camera.position = carPos + glm::vec3(-std::sin(rad)*CAM_DISTANCE, CAM_HEIGHT, -std::cos(rad)*CAM_DISTANCE);
    camera.direction = glm::normalize(carPos + glm::vec3(0,1,0) - camera.position);
}

void P5Scene::OnUpdate() {
    float dt = Time::deltaTime;
    UpdateAICars(dt);
    UpdateWanderCubes(dt);
    UpdatePlayerCar(dt);
}

// --- Rendering ---

void P5Scene::RenderDynamic(unsigned int shaderID) {
    // Player car
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "uModel"),
                       1, GL_FALSE, glm::value_ptr(GetPlayerCarModel()));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, loadedTextures[2]); // steelTex
    glUniform1i(glGetUniformLocation(shaderID, "uTexture"), 0);
    objectRenderer.BindAndDraw();

    // AI cars (use brick texture for contrast)
    for (const auto& ai : aiCars) {
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "uModel"),
                           1, GL_FALSE, glm::value_ptr(GetAICarModel(ai)));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, loadedTextures[0]); // brickTex
        glUniform1i(glGetUniformLocation(shaderID, "uTexture"), 0);
        objectRenderer.BindAndDraw();
    }

    // Wandering cubes (use wood texture)
    for (const auto& wc : wanderCubes) {
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "uModel"),
                           1, GL_FALSE, glm::value_ptr(GetWanderCubeModel(wc)));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, loadedTextures[1]); // woodTex
        glUniform1i(glGetUniformLocation(shaderID, "uTexture"), 0);
        objectRenderer.BindAndDraw();
    }
}

void P5Scene::OnRenderGeometry(unsigned int shaderID, const glm::mat4& lightMVP) {
    int loc = glGetUniformLocation(shaderID, "uLightMVP");

    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(lightMVP));
    road.DrawGeometry();

    for (const auto& obj : objects) {
        glm::mat4 model = glm::translate(glm::mat4(1), obj.position);
        model = glm::scale(model, obj.scale);
        model = glm::translate(model, glm::vec3(0, 0.5f, 0));
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(lightMVP * model));
        objectRenderer.BindAndDraw();
    }

    // Dynamic shadows
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(lightMVP * GetPlayerCarModel()));
    objectRenderer.BindAndDraw();
    for (const auto& ai : aiCars) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(lightMVP * GetAICarModel(ai)));
        objectRenderer.BindAndDraw();
    }
    for (const auto& wc : wanderCubes) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(lightMVP * GetWanderCubeModel(wc)));
        objectRenderer.BindAndDraw();
    }
}

void P5Scene::OnRender(const glm::mat4& view, const glm::mat4& projection) {
    if (config.useLighting) {
        // Road
        glUniformMatrix4fv(glGetUniformLocation(litShader.programID, "uModel"),
                           1, GL_FALSE, glm::value_ptr(glm::mat4(1)));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, road.GetTexture());
        glUniform1i(glGetUniformLocation(litShader.programID, "uTexture"), 0);
        road.DrawGeometry();

        // Static objects
        for (const auto& obj : objects) {
            glm::mat4 model = glm::translate(glm::mat4(1), obj.position);
            model = glm::scale(model, obj.scale);
            model = glm::translate(model, glm::vec3(0, 0.5f, 0));
            glUniformMatrix4fv(glGetUniformLocation(litShader.programID, "uModel"),
                               1, GL_FALSE, glm::value_ptr(model));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, obj.textureID);
            glUniform1i(glGetUniformLocation(litShader.programID, "uTexture"), 0);
            objectRenderer.BindAndDraw();
        }

        // All dynamic objects
        RenderDynamic(litShader.programID);
    } else {
        road.Render(view, projection);
        objectRenderer.Render(objects, view, projection);
    }

    // Collision flash
    if (collisionTimer > 0.0f) {
        ImGui::Begin("##collision", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
        ImGui::SetWindowPos(ImVec2(10, 10));
        ImGui::TextColored(ImVec4(1, 0, 0, collisionTimer / 0.3f), "COLLISION!");
        ImGui::End();
    }

    // HUD
    ImGui::Begin("Car");
    ImGui::Text("Speed: %.1f", carSpeed);
    ImGui::Text("Position: (%.1f, %.1f)", carPos.x, carPos.z);
    ImGui::Text("AI Cars: %d | Wanderers: %d", (int)aiCars.size(), (int)wanderCubes.size());
    ImGui::End();
}

void P5Scene::OnUnload() {
    road.Unload();
    objectRenderer.Unload();
    for (auto tex : loadedTextures) glDeleteTextures(1, &tex);
    loadedTextures.clear();
    objects.clear();
    staticColliders.clear();
    aiCars.clear();
    wanderCubes.clear();
}
```

---

### Task 3: Build and verify

**Step 1: Reconfigure and build**

Run: `export PATH="/c/msys64/ucrt64/bin:$PATH" && cmake -S . -B build -G Ninja && cmake --build build 2>&1`

Expected: clean build.

**Step 2: Manual verification**

Run the app, switch to "Scene 5" tab:
- [ ] Same environment as P3/P4 (buildings, trees, poles, road, lighting)
- [ ] Player car works with WASD, 3rd-person camera follows
- [ ] 2 AI cars orbit the road in opposite directions at different speeds
- [ ] AI cars have yaw matching their travel direction (tangent to ellipse)
- [ ] 5 wandering cubes drift around outside the road
- [ ] Wandering cubes bounce off buildings/trees when they collide
- [ ] Player car collides with AI cars (stops, shows COLLISION!)
- [ ] Player car collides with wandering cubes
- [ ] All dynamic objects cast shadows
- [ ] Lighting debug UI still works
