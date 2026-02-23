#pragma once
#include "scenes/scene.hpp"
#include <vector>

class SceneManager {
public:
    std::vector<Scene*> scenes;
    int activeIndex = -1;

    void RegisterScene(Scene* scene);
    void SwitchTo(int index);
    void Update();
    void Render();
    void RenderTabs();
    void UnloadAll();
};
