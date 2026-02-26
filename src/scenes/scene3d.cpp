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
