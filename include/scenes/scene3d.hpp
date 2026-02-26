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
