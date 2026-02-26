# Scene3D Modular Infrastructure Design

## Problem
Every 3D scene duplicates ~30 lines of boilerplate: camera setup, delta time calculation,
cursor lock/unlock logic, skybox/terrain loading/rendering, depth test enable/disable.
Additionally, the cursor unlock + ImGui tab click conflict causes scenes to immediately
re-lock when clicking tabs.

## Solution: Scene3D Base Class + Time Utility + Terrain Generator

### Architecture

```
Scene (abstract)
  +-- Scene3D (abstract) -- Camera, Skybox, Terrain, cursor, Time
        +-- P1Scene -- rotating cube
        +-- P2Scene -- street circuit (future)
        +-- P3-P5Scene -- future scenes
```

### 1. Time Struct (Unity-style)

```cpp
// include/utils/time.hpp
struct Time {
    static float deltaTime;  // seconds since last frame
    static float time;       // total elapsed seconds
    static void Update();    // called once per frame by Scene3D
};
```

- `Time::Update()` computes deltaTime from glfwGetTime()
- Accessible globally, no need to pass deltaTime around

### 2. TerrainGenerator Interface

```cpp
// include/scenes/terrain_generator.hpp
class TerrainGenerator {
public:
    virtual ~TerrainGenerator() = default;
    virtual float GetHeight(float x, float z) const = 0;
};

class FlatGenerator : public TerrainGenerator {
    float height;
public:
    FlatGenerator(float h = 1.0f) : height(h) {}
    float GetHeight(float x, float z) const override { return height; }
};
```

- Terrain::Load() uses FlatGenerator by default
- Terrain::Load(TerrainGenerator* gen) uses custom generator
- Mesh generation calls generator->GetHeight(x, z) per vertex

### 3. Scene3D Base Class

```cpp
// include/scenes/scene3d.hpp
class Scene3D : public Scene {
public:
    Scene3D(const std::string& name,
            glm::vec3 cameraPos = glm::vec3(0, 2, 3),
            float fov = 45.0f,
            float nearPlane = 0.1f,
            float farPlane = 100.0f);

protected:
    Camera camera;
    Skybox skybox;
    Terrain terrain;
    float fov, nearPlane, farPlane;
    bool cursorLocked = true;

    // Hooks for subclasses (pure virtual)
    virtual void OnLoad() = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender(const glm::mat4& view, const glm::mat4& projection) = 0;
    virtual void OnUnload() = 0;

private:
    // Scene interface (final - subclasses use On* hooks)
    void Load() override final;
    void Update() override final;
    void Render() override final;
    void Unload() override final;
};
```

#### Load() flow:
1. Enable GL_DEPTH_TEST
2. Lock cursor (GLFW_CURSOR_DISABLED)
3. Initialize Time
4. Load skybox, terrain
5. Call OnLoad()

#### Update() flow:
1. Time::Update()
2. Handle Delete key (close window)
3. Handle ESC (unlock cursor)
4. Handle LMB (relock cursor) -- WITH ImGui::GetIO().WantCaptureMouse guard
5. If cursor locked: camera.Update(window, Time::deltaTime)
6. Call OnUpdate()

#### Render() flow:
1. Build view matrix from camera
2. Build projection matrix from fov/aspect/near/far
3. Render skybox(view, projection)
4. Render terrain(view, projection)
5. Call OnRender(view, projection)

#### Unload() flow:
1. Call OnUnload()
2. Unload skybox, terrain
3. Disable GL_DEPTH_TEST
4. Restore cursor to normal

### 4. ImGui Click Fix

The cursor relock check adds `!ImGui::GetIO().WantCaptureMouse` so clicking
ImGui tabs doesn't re-lock the cursor and steal the click from the tab bar.

### 5. Refactored P1Scene

```cpp
class P1Scene : public Scene3D {
public:
    P1Scene() : Scene3D("Skybox", glm::vec3(0, 2, 3)) {}
    void OnLoad() override;    // load cube shader + mesh
    void OnUpdate() override;  // rotate cube, hot-reload shader
    void OnRender(const glm::mat4& view, const glm::mat4& projection) override;
    void OnUnload() override;  // cleanup cube
private:
    Shader shader;
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    float rotationAngle = 0.0f;
};
```

### 6. Files Changed

| Action | File |
|--------|------|
| Create | include/utils/time.hpp |
| Create | src/utils/time.cpp |
| Create | include/scenes/terrain_generator.hpp |
| Create | include/scenes/scene3d.hpp |
| Create | src/scenes/scene3d.cpp |
| Modify | include/scenes/terrain.hpp -- accept TerrainGenerator* |
| Modify | src/scenes/terrain.cpp -- use generator in mesh loop |
| Modify | include/scenes/p1_scene.hpp -- inherit Scene3D |
| Modify | src/scenes/p1_scene.cpp -- use On* hooks |
| Modify | CMakeLists.txt -- add new sources |
