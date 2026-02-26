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
    unsigned int steelTex = objectRenderer.LoadTexture("resources/textures/objects/steel.jpg");

    loadedTextures.push_back(brickTex);
    loadedTextures.push_back(woodTex);
    loadedTextures.push_back(steelTex);

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
        // Trunk
        objects.push_back({ glm::vec3(x, 1.0f, z), glm::vec3(1, height, 1), woodTex });
        // 4 branches at the top, tilted 45 degrees outward
        float treeTop = 1.0f + height;
        for (int b = 0; b < 4; b++) {
            float yaw = glm::radians(b * 90.0f);
            objects.push_back({ glm::vec3(x, treeTop, z), glm::vec3(0.4f, 3.0f, 0.4f), woodTex,
                                glm::vec3(0.0f, yaw, glm::radians(-45.0f)) });
        }
    }

    // 4 street lamp poles (L-shaped) at the spot light positions
    float streetlightAngles[] = { 0.0f, 90.0f, 180.0f, 270.0f };
    for (int i = 0; i < 4; i++) {
        float angle = glm::radians(streetlightAngles[i]);
        // Light hangs at middle of road
        float lightX = 37.5f * cos(angle);
        float lightZ = 27.5f * sin(angle);
        // Pole base at outer road edge
        float poleX = 40.0f * cos(angle);
        float poleZ = 30.0f * sin(angle);

        float poleHeight = 12.0f;
        float armThickness = 0.3f;

        // Vertical pole
        objects.push_back({ glm::vec3(poleX, 0.0f, poleZ),
                            glm::vec3(armThickness, poleHeight, armThickness), steelTex });

        // Horizontal arm connecting pole top to light position
        float dx = lightX - poleX;
        float dz = lightZ - poleZ;
        float armLen = std::sqrt(dx * dx + dz * dz);
        float armX = (poleX + lightX) * 0.5f;
        float armZ = (poleZ + lightZ) * 0.5f;
        // Orient arm along whichever axis has the larger delta
        glm::vec3 armScale = (std::abs(dx) > std::abs(dz))
            ? glm::vec3(armLen, armThickness, armThickness)
            : glm::vec3(armThickness, armThickness, armLen);

        objects.push_back({ glm::vec3(armX, poleHeight - armThickness, armZ),
                            armScale, steelTex });
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
        float x = 37.5f * cos(angle);
        float z = 27.5f * sin(angle);

        SpotLight spot;
        spot.position = glm::vec3(x, 12.0f, z);
        spot.direction = glm::vec3(0.0f, -1.0f, 0.0f);
        spot.color = glm::vec3(1.0f, 0.9f, 0.7f);
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
        glm::mat4 model = ModelMatrixFromObject(obj);
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(lightMVP * model));
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
            glm::mat4 model = ModelMatrixFromObject(obj);
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
