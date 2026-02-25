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
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, 0));
    ImGui::Begin("##SceneBar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

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

    ImGui::End();
}

void SceneManager::UnloadAll() {
    for (auto* scene : scenes) {
        if (scene->loaded) {
            scene->Unload();
            scene->loaded = false;
        }
    }
}
