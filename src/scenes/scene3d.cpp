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

    if (config.useLighting) {
        RenderLit(view, projection);
        RenderLightingDebugUI();
    } else {
        RenderUnlit(view, projection);
    }
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

void Scene3D::RenderLightingDebugUI() {
    ImGui::Begin("Lighting");

    // Ambient
    ImGui::ColorEdit3("Ambient", &lighting.ambientColor.x);

    // Sun
    if (ImGui::CollapsingHeader("Sun", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Sun Intensity", &lighting.sun.intensity, 0.0f, 5.0f);
        ImGui::ColorEdit3("Sun Color", &lighting.sun.color.x);
        ImGui::DragFloat3("Sun Direction", &lighting.sun.direction.x, 0.01f, -1.0f, 1.0f);
    }

    // Spot lights
    if (!lighting.spotLights.empty() && ImGui::CollapsingHeader("Spot Lights")) {
        for (int i = 0; i < (int)lighting.spotLights.size(); i++) {
            ImGui::PushID(i);
            char label[32];
            snprintf(label, sizeof(label), "Spot %d", i);
            if (ImGui::TreeNode(label)) {
                ImGui::DragFloat3("Position", &lighting.spotLights[i].position.x, 0.5f);
                ImGui::DragFloat3("Direction", &lighting.spotLights[i].direction.x, 0.01f, -1.0f, 1.0f);
                ImGui::SliderFloat("Intensity", &lighting.spotLights[i].intensity, 0.0f, 10.0f);
                ImGui::ColorEdit3("Color", &lighting.spotLights[i].color.x);
                ImGui::SliderFloat("Range", &lighting.spotLights[i].range, 1.0f, 100.0f);
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }

    // Point lights
    if (!lighting.pointLights.empty() && ImGui::CollapsingHeader("Point Lights")) {
        for (int i = 0; i < (int)lighting.pointLights.size(); i++) {
            ImGui::PushID(1000 + i);
            char label[32];
            snprintf(label, sizeof(label), "Point %d", i);
            if (ImGui::TreeNode(label)) {
                ImGui::SliderFloat("Intensity", &lighting.pointLights[i].intensity, 0.0f, 10.0f);
                ImGui::ColorEdit3("Color", &lighting.pointLights[i].color.x);
                ImGui::DragFloat3("Position", &lighting.pointLights[i].position.x, 0.5f);
                ImGui::SliderFloat("Constant", &lighting.pointLights[i].constant, 0.0f, 2.0f);
                ImGui::SliderFloat("Linear", &lighting.pointLights[i].linear, 0.0f, 1.0f);
                ImGui::SliderFloat("Quadratic", &lighting.pointLights[i].quadratic, 0.0f, 0.5f);
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }

    ImGui::End();
}
