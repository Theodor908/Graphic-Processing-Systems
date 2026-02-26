#pragma once
#include "scenes/scene3d.hpp"
#include "scenes/road.hpp"
#include "scenes/static_object.hpp"

class P3Scene : public Scene3D {
public:
    P3Scene();

    void OnLoad() override;
    void OnUpdate() override;
    void OnRender(const glm::mat4& view, const glm::mat4& projection) override;
    void OnUnload() override;
    void OnRenderGeometry(unsigned int shaderID, const glm::mat4& lightMVP) override;

private:
    Road road;
    StaticObjectRenderer objectRenderer;
    std::vector<ObjectInstance> objects;
    std::vector<unsigned int> loadedTextures;

    void SetupObjects();
    void SetupLights();
};
