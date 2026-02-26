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

    // Override to draw scene geometry for shadow passes
    virtual void OnRenderGeometry(unsigned int shaderID, const glm::mat4& lightMVP) {}

private:
    void Load() override final;
    void Update() override final;
    void Render() override final;
    void Unload() override final;

    void RenderLit(const glm::mat4& view, const glm::mat4& projection);
    void RenderUnlit(const glm::mat4& view, const glm::mat4& projection);
    void RenderLightingDebugUI();
};
