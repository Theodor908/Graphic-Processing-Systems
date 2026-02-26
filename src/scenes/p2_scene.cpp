#include "scenes/p2_scene.hpp"
#include <cmath>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

void P2Scene::OnLoad() {
    road.Load();
    objectRenderer.Load();
    SetupObjects();
}

void P2Scene::SetupObjects() {
    unsigned int brickTex = objectRenderer.LoadTexture("resources/textures/objects/building.jpg");
    unsigned int woodTex  = objectRenderer.LoadTexture("resources/textures/objects/tree_trunk.jpg");
    loadedTextures.push_back(brickTex);
    loadedTextures.push_back(woodTex);

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
        float treeTop = 1.0f + height;
        for (int b = 0; b < 4; b++) {
            float yaw = glm::radians(b * 90.0f);
            objects.push_back({ glm::vec3(x, treeTop, z), glm::vec3(0.4f, 3.0f, 0.4f), woodTex,
                                glm::vec3(0.0f, yaw, glm::radians(-45.0f)) });
        }
    }
}

void P2Scene::OnUpdate() {
}

void P2Scene::OnRender(const glm::mat4& view, const glm::mat4& projection) {
    road.Render(view, projection);
    objectRenderer.Render(objects, view, projection);
}

void P2Scene::OnUnload() {
    road.Unload();
    objectRenderer.Unload();

    // Delete textures (each only once)
    for (auto tex : loadedTextures) {
        glDeleteTextures(1, &tex);
    }
    loadedTextures.clear();
    objects.clear();
}
