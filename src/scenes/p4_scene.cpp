#include "scenes/p4_scene.hpp"
#include "lighting/light.hpp"
#include "utils/time.hpp"
#include "glad.h"
#include "glfw3.h"
#include "imgui.h"
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
    unsigned int brickTex = objectRenderer.LoadTexture("resources/textures/objects/building.jpg");
    unsigned int woodTex  = objectRenderer.LoadTexture("resources/textures/objects/tree_trunk.jpg");
    unsigned int steelTex = objectRenderer.LoadTexture("resources/textures/objects/steel.jpg");
    unsigned int carTex   = objectRenderer.LoadTexture("resources/textures/objects/car.jpg");

    loadedTextures.push_back(brickTex);
    loadedTextures.push_back(woodTex);
    loadedTextures.push_back(steelTex);
    loadedTextures.push_back(carTex);

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
        float treeTop = 1.0f + height;
        for (int b = 0; b < 4; b++) {
            float yaw = glm::radians(b * 90.0f);
            objects.push_back({ glm::vec3(x, treeTop, z), glm::vec3(0.4f, 3.0f, 0.4f), woodTex,
                                glm::vec3(0.0f, yaw, glm::radians(-45.0f)) });
        }
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
}

glm::mat4 P4Scene::GetCarModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, carPos);
    model = glm::rotate(model, glm::radians(carYaw), glm::vec3(0, 1, 0));
    model = glm::scale(model, carScale);
    model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f));
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
    AABB carBox = AABBFromCar(newPos, carHalf);
    bool hitX = false;
    for (const auto& col : colliders) {
        if (carBox.Overlaps(col)) { hitX = true; break; }
    }
    if (hitX) {
        newPos.x = carPos.x;
        collisionTimer = 0.3f;
    }

    // Try Z
    newPos.z += movement.z;
    carBox = AABBFromCar(newPos, carHalf);
    bool hitZ = false;
    for (const auto& col : colliders) {
        if (carBox.Overlaps(col)) { hitZ = true; break; }
    }
    if (hitZ) {
        newPos.z = carPos.z;
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
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, loadedTextures[3]); 
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
        glm::mat4 model = ModelMatrixFromObject(obj);
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
            glm::mat4 model = ModelMatrixFromObject(obj);
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

    // Car HUD
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
