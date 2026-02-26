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
    .name = "Random and AI Cars",
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

    // 5 wandering cubes spawned between road and buildings
    srand(42);
    for (int i = 0; i < 5; i++) {
        float angle = glm::radians(i * 72.0f + 15.0f);
        float radius = 42.0f + (rand() % 4);
        glm::vec3 pos(radius * std::cos(angle), 0.0f, radius * std::sin(angle));

        float dirAngle = glm::radians((float)(rand() % 360));
        glm::vec3 dir(std::cos(dirAngle), 0.0f, std::sin(dirAngle));

        wanderCubes.push_back({ pos, dir, 3.0f + (float)(rand() % 3) });
    }
}

void P5Scene::SetupObjects() {
    unsigned int brickTex = objectRenderer.LoadTexture("resources/textures/objects/building.jpg");
    unsigned int woodTex  = objectRenderer.LoadTexture("resources/textures/objects/tree_trunk.jpg");
    unsigned int steelTex = objectRenderer.LoadTexture("resources/textures/objects/steel.jpg");
    unsigned int carTex   = objectRenderer.LoadTexture("resources/textures/objects/car.jpg");
    unsigned int cubeTex  = objectRenderer.LoadTexture("resources/textures/objects/cube.jpg");

    loadedTextures.push_back(brickTex);
    loadedTextures.push_back(woodTex);
    loadedTextures.push_back(steelTex);
    loadedTextures.push_back(carTex);
    loadedTextures.push_back(cubeTex);

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
        float treeTop = 1.0f + height;
        for (int b = 0; b < 4; b++) {
            float yaw = glm::radians(b * 90.0f);
            objects.push_back({ glm::vec3(x, treeTop, z), glm::vec3(0.4f, 3.0f, 0.4f), woodTex,
                                glm::vec3(0.0f, yaw, glm::radians(-45.0f)) });
        }
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

        // Yaw from ellipse tangent
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
            wc.dir.x = -wc.dir.x;
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
            wc.dir.z = -wc.dir.z;
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

    // Road detection: on road if between inner ellipse (35,25) and outer ellipse (40,30)
    float ex = carPos.x, ez = carPos.z;
    float eOuter = (ex/40.0f)*(ex/40.0f) + (ez/30.0f)*(ez/30.0f);
    float eInner = (ex/35.0f)*(ex/35.0f) + (ez/25.0f)*(ez/25.0f);
    bool onRoad = (eOuter <= 1.0f && eInner >= 1.0f);
    float speedMult = onRoad ? 1.5f : 1.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) carSpeed += CAR_ACCEL * dt;
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) carSpeed -= CAR_BRAKE * dt;
    else {
        if (carSpeed > 0) carSpeed = glm::max(0.0f, carSpeed - CAR_FRICTION * dt);
        else if (carSpeed < 0) carSpeed = glm::min(0.0f, carSpeed + CAR_FRICTION * dt);
    }
    float maxSpd = CAR_MAX_SPEED * speedMult;
    carSpeed = glm::clamp(carSpeed, -maxSpd * 0.5f, maxSpd);

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
    glBindTexture(GL_TEXTURE_2D, loadedTextures[3]); // carTex
    glUniform1i(glGetUniformLocation(shaderID, "uTexture"), 0);
    objectRenderer.BindAndDraw();

    // AI cars (brick texture for contrast)
    for (const auto& ai : aiCars) {
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "uModel"),
                           1, GL_FALSE, glm::value_ptr(GetAICarModel(ai)));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, loadedTextures[3]); // carTex
        glUniform1i(glGetUniformLocation(shaderID, "uTexture"), 0);
        objectRenderer.BindAndDraw();
    }

    // Wandering cubes (rainbow texture)
    for (const auto& wc : wanderCubes) {
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "uModel"),
                           1, GL_FALSE, glm::value_ptr(GetWanderCubeModel(wc)));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, loadedTextures[4]); // cubeTex
        glUniform1i(glGetUniformLocation(shaderID, "uTexture"), 0);
        objectRenderer.BindAndDraw();
    }
}

void P5Scene::OnRenderGeometry(unsigned int shaderID, const glm::mat4& lightMVP) {
    int loc = glGetUniformLocation(shaderID, "uLightMVP");

    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(lightMVP));
    road.DrawGeometry();

    for (const auto& obj : objects) {
        glm::mat4 model = ModelMatrixFromObject(obj);
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
            glm::mat4 model = ModelMatrixFromObject(obj);
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
