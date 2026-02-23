#include "scenes/scene_manager.hpp"
#include "imgui.h"

void SceneManager::RegisterScene(Scene* scene) {
    scenes.push_back(scene);
}

void SceneManager::SwitchTo(int index) {
    if (activeIndex >= 0 && scenes[activeIndex]->loaded) {
        scenes[activeIndex]->Unload();
        scenes[activeIndex]->loaded = false;
    }
    activeIndex = index;
    scenes[activeIndex]->Load();
    scenes[activeIndex]->loaded = true;
}

void SceneManager::Update() {
    if (activeIndex >= 0) {
        scenes[activeIndex]->Update();
    }
}

void SceneManager::Render() {
    if (activeIndex >= 0) {
        scenes[activeIndex]->Render();
    }
}

void SceneManager::RenderTabs() {
    if (ImGui::BeginTabBar("SceneTabs")) {
        for (int i = 0; i < (int)scenes.size(); i++) {
            if (ImGui::BeginTabItem(scenes[i]->name.c_str())) {
                if (i != activeIndex) {
                    SwitchTo(i);
                }
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
}

void SceneManager::UnloadAll() {
    for (auto* scene : scenes) {
        if (scene->loaded) {
            scene->Unload();
            scene->loaded = false;
        }
    }
}
