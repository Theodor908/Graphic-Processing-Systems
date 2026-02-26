#pragma once
#include "scenes/scene3d.hpp"
#include "scenes/road.hpp"
#include "scenes/static_object.hpp"
#include "collision/aabb.hpp"

class P4Scene : public Scene3D {
public:
    P4Scene();

    void OnLoad() override;
    void OnUpdate() override;
    void OnRender(const glm::mat4& view, const glm::mat4& projection) override;
    void OnUnload() override;
    void OnRenderGeometry(unsigned int shaderID, const glm::mat4& lightMVP) override;

private:
    Road road;
    StaticObjectRenderer objectRenderer;
    std::vector<ObjectInstance> objects;
    std::vector<AABB> colliders;
    std::vector<unsigned int> loadedTextures;

    // Car state
    glm::vec3 carPos = glm::vec3(37.5f, 0.0f, 0.0f);
    float carYaw = 0.0f;
    float carSpeed = 0.0f;
    float collisionTimer = 0.0f;

    static constexpr float CAR_MAX_SPEED = 15.0f;
    static constexpr float CAR_ACCEL = 20.0f;
    static constexpr float CAR_BRAKE = 30.0f;
    static constexpr float CAR_FRICTION = 8.0f;
    static constexpr float CAR_TURN_SPEED = 120.0f;
    static constexpr float CAM_DISTANCE = 12.0f;
    static constexpr float CAM_HEIGHT = 6.0f;

    glm::vec3 carScale = glm::vec3(1.5f, 1.0f, 2.5f);
    glm::vec3 carHalf = glm::vec3(1.0f, 1.5f, 1.0f);

    void SetupObjects();
    void SetupLights();
    void UpdateCar(float dt);
    void RenderCar(unsigned int shaderID);
    glm::mat4 GetCarModelMatrix() const;
};
