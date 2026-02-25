# Scene System Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add a multi-scene tab system so 5 assignments coexist in one application, switchable via ImGui tabs with per-scene load/unload.

**Architecture:** Abstract `Scene` base class with lifecycle methods. `SceneManager` owns all scenes, renders an ImGui tab bar, and calls Load/Unload on switch. `GameWindow` becomes a thin shell delegating to SceneManager. Existing template code migrates into P1Scene.

**Tech Stack:** C++, OpenGL 3.3, GLFW, ImGui (already integrated)

---

### Task 1: Create Scene Base Class

**Files:**
- Create: `include/scenes/scene.hpp`

**Step 1: Create the abstract Scene class**

Write a header-only abstract class with:
- `#pragma once` guard
- Includes: `<string>`
- Public members: `std::string name`, `bool loaded` (default `false`)
- Constructor taking `std::string name`
- Pure virtual methods: `Load()`, `Update()`, `Render()`, `Unload()`
- Virtual destructor (default)

```cpp
#pragma once
#include <string>

class Scene {
public:
    std::string name;
    bool loaded = false;

    Scene(const std::string& name) : name(name) {}
    virtual ~Scene() = default;

    virtual void Load() = 0;
    virtual void Update() = 0;
    virtual void Render() = 0;
    virtual void Unload() = 0;
};
```

**Step 2: Verify it compiles**

Run: `cd build && cmake -G "MinGW Makefiles" .. && cmake --build .`
Expected: Compiles successfully (no source files include it yet, but header should be valid)

**Step 3: Commit**

```bash
git add include/scenes/scene.hpp
git commit -m "feat: add abstract Scene base class"
```

---

### Task 2: Create Scene Manager

**Files:**
- Create: `include/scenes/scene_manager.hpp`
- Create: `src/scenes/scene_manager.cpp`

**Step 1: Write the SceneManager header**

```cpp
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
```

**Step 2: Write the SceneManager implementation**

In `src/scenes/scene_manager.cpp`:

- `RegisterScene`: push_back the scene pointer into `scenes`
- `SwitchTo(index)`: if `activeIndex >= 0` and current scene is loaded, call `Unload()` on it and set `loaded = false`. Then set `activeIndex = index`, call `Load()` on the new scene, set `loaded = true`.
- `Update()`: if `activeIndex >= 0`, call `scenes[activeIndex]->Update()`
- `Render()`: if `activeIndex >= 0`, call `scenes[activeIndex]->Render()`
- `RenderTabs()`: Use `ImGui::BeginTabBar("SceneTabs")`. Loop through `scenes`, for each call `ImGui::BeginTabItem(scenes[i]->name.c_str())`. When a tab item is active and `i != activeIndex`, call `SwitchTo(i)`. Call `ImGui::EndTabItem()` and `ImGui::EndTabBar()`.
- `UnloadAll()`: loop all scenes, if `loaded`, call `Unload()` and set `loaded = false`

Important: `scene_manager.cpp` needs to include ImGui headers for the tab bar:
```cpp
#include "scenes/scene_manager.hpp"
#include "imgui.h"
```

**Step 3: Verify it compiles**

Run: `cd build && cmake --build .`
Expected: Compiles (nothing uses SceneManager yet, but the translation unit should compile)

**Step 4: Commit**

```bash
git add include/scenes/scene_manager.hpp src/scenes/scene_manager.cpp
git commit -m "feat: add SceneManager with tab switching"
```

---

### Task 3: Create P1Scene (migrate existing template code)

**Files:**
- Create: `include/scenes/p1_scene.hpp`
- Create: `src/scenes/p1_scene.cpp`

**Step 1: Write P1Scene header**

```cpp
#pragma once
#include "scenes/scene.hpp"
#include "shaders/shader.hpp"

class P1Scene : public Scene {
public:
    P1Scene() : Scene("P1 - Cube Scene") {}

    void Load() override;
    void Update() override;
    void Render() override;
    void Unload() override;

private:
    Shader shader;
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
};
```

Note: The global variables `Shader s`, `VAO`, `VBO`, `EBO` from `game_window.cpp:6-9` become private members here.

**Step 2: Write P1Scene implementation**

Move the logic from `game_window.cpp` into P1Scene methods:

- `Load()`: Take the content of `GameWindow::LoadContent()` lines 39-76 (shader loading, vertex data, VAO/VBO/EBO creation). Do NOT include the ImGui init or framebuffer callback — those stay in GameWindow.
- `Update()`: Take `game_window.cpp:81` — `shader.ReloadFromFile()`
- `Render()`: Take `game_window.cpp:86-102` — bind VAO, use program, draw elements. Do NOT include ImGui frame begin/end, clear color, swap buffers, or poll events — those stay in GameWindow.
- `Unload()`: Delete OpenGL resources — `glDeleteVertexArrays(1, &VAO)`, `glDeleteBuffers(1, &VBO)`, `glDeleteBuffers(1, &EBO)`, `shader.Unload()`. Reset handles to 0.

Include `glad.h` in the cpp file for OpenGL calls.

**Step 3: Commit**

```bash
git add include/scenes/p1_scene.hpp src/scenes/p1_scene.cpp
git commit -m "feat: add P1Scene with migrated template rendering"
```

---

### Task 4: Create P2-P5 Scene Stubs

**Files:**
- Create: `include/scenes/p2_scene.hpp`, `src/scenes/p2_scene.cpp`
- Create: `include/scenes/p3_scene.hpp`, `src/scenes/p3_scene.cpp`
- Create: `include/scenes/p4_scene.hpp`, `src/scenes/p4_scene.cpp`
- Create: `include/scenes/p5_scene.hpp`, `src/scenes/p5_scene.cpp`

**Step 1: Write each scene as a minimal stub**

Each follows the same pattern (example for P2):

Header:
```cpp
#pragma once
#include "scenes/scene.hpp"

class P2Scene : public Scene {
public:
    P2Scene() : Scene("P2 - [Title]") {}
    void Load() override;
    void Update() override;
    void Render() override;
    void Unload() override;
};
```

Implementation — all four methods are empty:
```cpp
#include "scenes/p2_scene.hpp"

void P2Scene::Load() {}
void P2Scene::Update() {}
void P2Scene::Render() {}
void P2Scene::Unload() {}
```

Repeat for P3, P4, P5 with appropriate names ("P3 - [Title]", etc.).

**Step 2: Commit**

```bash
git add include/scenes/p2_scene.hpp src/scenes/p2_scene.cpp \
        include/scenes/p3_scene.hpp src/scenes/p3_scene.cpp \
        include/scenes/p4_scene.hpp src/scenes/p4_scene.cpp \
        include/scenes/p5_scene.hpp src/scenes/p5_scene.cpp
git commit -m "feat: add P2-P5 scene stubs"
```

---

### Task 5: Rewire GameWindow to Use SceneManager

**Files:**
- Modify: `include/display/game_window.hpp` (add SceneManager member)
- Modify: `src/display/game_window.cpp` (replace body with delegation)

**Step 1: Update game_window.hpp**

Add `#include "scenes/scene_manager.hpp"` and a `SceneManager sceneManager` member to GameWindow.

```cpp
#include "display/base_window.hpp"
#include "scenes/scene_manager.hpp"

class GameWindow : public BaseWindow {
public:
    GameWindow(int width, int height, std::string title) : BaseWindow(width, height, title) {};
    void Initialize();
    void LoadContent();
    void Update();
    void Render();
    void Unload();

private:
    SceneManager sceneManager;
};
```

**Step 2: Rewrite game_window.cpp**

Remove the old globals (`Shader s`, `VAO`, `VBO`, `EBO`).
Keep `FramebufferSizeCallback` as-is.
Keep `Initialize()` as-is (GLFW hints).

Rewrite `LoadContent()`:
- Keep ImGui init (lines 28-37)
- Remove shader/vertex/buffer code (lines 39-76)
- Add: include all scene headers, register scenes, switch to first:
```cpp
#include "scenes/p1_scene.hpp"
#include "scenes/p2_scene.hpp"
// ... etc

void GameWindow::LoadContent() {
    glfwSetFramebufferSizeCallback(this->windowHandle, FramebufferSizeCallback);

    // Initialize imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(this->windowHandle, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Register scenes
    sceneManager.RegisterScene(new P1Scene());
    sceneManager.RegisterScene(new P2Scene());
    sceneManager.RegisterScene(new P3Scene());
    sceneManager.RegisterScene(new P4Scene());
    sceneManager.RegisterScene(new P5Scene());
    sceneManager.SwitchTo(0);
}
```

Rewrite `Update()`:
```cpp
void GameWindow::Update() {
    sceneManager.Update();
}
```

Rewrite `Render()`:
```cpp
void GameWindow::Render() {
    // Begin ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Clear screen
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Tab bar and active scene
    sceneManager.RenderTabs();
    sceneManager.Render();

    // End ImGui frame
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(this->windowHandle);
    glfwPollEvents();
}
```

Rewrite `Unload()`:
```cpp
void GameWindow::Unload() {
    sceneManager.UnloadAll();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
```

**Step 3: Build and run**

Run: `cd build && cmake --build . && ./opengl-imgui-cmake-template.exe`
Expected: Window opens with 5 tabs at the top. P1 tab shows the original striped square. P2-P5 tabs show empty (just the dark clear color). Switching tabs is instant.

**Step 4: Commit**

```bash
git add include/display/game_window.hpp src/display/game_window.cpp
git commit -m "feat: rewire GameWindow to use SceneManager with 5 tabs"
```

---

### Task 6: Verify and Polish

**Step 1: Test tab switching**

- Click each tab P1 through P5 — no crashes, no OpenGL errors
- Switch back to P1 — the square should reappear (Load/Unload cycle works)
- Check console for shader load messages when switching to/from P1

**Step 2: Verify no memory leaks on exit**

- Run the app, switch tabs a few times, close the window
- Should exit cleanly with return code 0

**Step 3: Final commit**

```bash
git add -A
git commit -m "feat: complete scene system with 5-tab switching"
```
