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
        objects.push_back({ glm::vec3(x, 1.0f, z), glm::vec3(1, height, 1), woodTex });
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
        float x = 37.5f * cos(angle);  // middle of road
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

    // 3 point lights around the scene — warm, cool, accent
    glm::vec3 pointColors[] = {
        glm::vec3(1.0f, 0.6f, 0.3f),   // warm orange
        glm::vec3(0.3f, 0.6f, 1.0f),   // cool blue
        glm::vec3(0.5f, 1.0f, 0.5f)    // green accent
    };
    float pointAngles[] = { 60.0f, 180.0f, 300.0f };
    for (int i = 0; i < 3; i++) {
        float angle = glm::radians(pointAngles[i]);
        PointLight pt;
        pt.position = glm::vec3(25.0f * cos(angle), 5.0f, 25.0f * sin(angle));
        pt.color = pointColors[i];
        pt.intensity = 3.0f;
        pt.constant = 1.0f;
        pt.linear = 0.09f;
        pt.quadratic = 0.032f;
        lighting.AddPointLight(pt);
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
