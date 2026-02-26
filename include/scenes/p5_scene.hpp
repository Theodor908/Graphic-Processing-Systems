#pragma once
#include "scenes/scene3d.hpp"
#include "scenes/road.hpp"
#include "scenes/static_object.hpp"
#include "collision/aabb.hpp"
#include <vector>

struct AICar {
    float angle;       // current position on ellipse (radians)
    float speed;       // angular speed (radians/sec)
    glm::vec3 pos;
    float yaw;
};

struct WanderCube {
    glm::vec3 pos;
    glm::vec3 dir;     // normalized XZ direction
    float speed;
};

class P5Scene : public Scene3D {
public:
    P5Scene();

    void OnLoad() override;
    void OnUpdate() override;
    void OnRender(const glm::mat4& view, const glm::mat4& projection) override;
    void OnUnload() override;
    void OnRenderGeometry(unsigned int shaderID, const glm::mat4& lightMVP) override;

private:
    Road road;
    StaticObjectRenderer objectRenderer;
    std::vector<ObjectInstance> objects;
    std::vector<AABB> staticColliders;
    std::vector<unsigned int> loadedTextures;

    // Player car
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

    // AI cars
    std::vector<AICar> aiCars;
    glm::vec3 aiCarScale = glm::vec3(1.5f, 1.0f, 2.5f);
    glm::vec3 aiCarHalf = glm::vec3(1.0f, 1.5f, 1.0f);

    // Wandering cubes
    std::vector<WanderCube> wanderCubes;
    glm::vec3 wanderScale = glm::vec3(2.0f, 2.0f, 2.0f);
    glm::vec3 wanderHalf = glm::vec3(1.0f, 1.5f, 1.0f);

    static constexpr float ROAD_RX = 37.5f;
    static constexpr float ROAD_RZ = 27.5f;

    void SetupObjects();
    void SetupLights();
    void UpdatePlayerCar(float dt);
    void UpdateAICars(float dt);
    void UpdateWanderCubes(float dt);
    void CollectDynamicColliders(std::vector<AABB>& out) const;
    glm::mat4 GetPlayerCarModel() const;
    glm::mat4 GetAICarModel(const AICar& ai) const;
    glm::mat4 GetWanderCubeModel(const WanderCube& wc) const;
    void RenderDynamic(unsigned int shaderID);
};
