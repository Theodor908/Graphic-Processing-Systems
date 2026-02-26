#pragma once
#include "scenes/scene3d.hpp"
#include "scenes/road.hpp"
#include "scenes/static_object.hpp"

class P2Scene : public Scene3D {
public:
    P2Scene() : Scene3D({.name = "Street", .cameraPos = glm::vec3(0.0f, 15.0f, 50.0f)}) {}

    void OnLoad() override;
    void OnUpdate() override;
    void OnRender(const glm::mat4& view, const glm::mat4& projection) override;
    void OnUnload() override;

private:
    Road road;
    StaticObjectRenderer objectRenderer;
    std::vector<ObjectInstance> objects;
    std::vector<unsigned int> loadedTextures;

    void SetupObjects();
};
