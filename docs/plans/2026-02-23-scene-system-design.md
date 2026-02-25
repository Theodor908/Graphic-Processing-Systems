# Scene System Design

## Goal
Multi-scene application with 5 tabs (one per assignment), using ImGui tab bar for switching. Each scene has its own isolated OpenGL state with load/unload on switch.

## Architecture: Scene Interface + Scene Manager (Approach A)

### Scene Base Class (`include/scenes/scene.hpp`)
Abstract class with lifecycle methods:
- `name: string` — displayed in tab
- `loaded: bool` — tracks resource state
- `Load()` — create VAOs, VBOs, shaders, textures (on switch TO)
- `Update(float dt)` — per-frame logic
- `Render()` — per-frame drawing
- `Unload()` — delete VAOs, VBOs, shaders, textures (on switch AWAY)

### Scene Manager (`include/scenes/scene_manager.hpp`)
Owns and coordinates scenes:
- `scenes: vector<Scene*>` — all 5 registered scenes
- `activeIndex: int` — currently active scene
- `RegisterScene(Scene*)` — adds scene to list
- `SwitchTo(int index)` — Unload() current, Load() new, update index
- `Update(float dt)` — delegates to active scene
- `Render()` — delegates to active scene
- `RenderTabs()` — ImGui::BeginTabBar/BeginTabItem, triggers SwitchTo() on click
- `UnloadAll()` — cleanup on exit

### GameWindow Integration
GameWindow simplified to thin shell:
- `LoadContent()` — init ImGui, create SceneManager, register 5 scenes, SwitchTo(0)
- `Update()` — sceneManager.Update(dt)
- `Render()` — clear, RenderTabs(), Render(), ImGui render, swap
- `Unload()` — UnloadAll(), shutdown ImGui

Current template code (square, shader, VAO/VBO/EBO) moves into P1Scene. Global variables become P1Scene members.

### Concrete Scenes
- `P1Scene` — Assignment 1 (cube scene with textures, horizon, terrain)
- `P2Scene` through `P5Scene` — stubs with empty lifecycle methods

### File Structure
```
include/scenes/   — scene.hpp, scene_manager.hpp, p1-p5_scene.hpp
src/scenes/       — scene_manager.cpp, p1-p5_scene.cpp
```
No CMakeLists.txt changes needed (GLOB_RECURSE handles new files).

## Decisions
- Load/unload on switch (not all-at-startup) per user preference
- ImGui tab bar for UI (already integrated in template)
- 5 scenes, fixed count
