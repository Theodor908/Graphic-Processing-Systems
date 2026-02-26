# P3: Configurable Scene3D + Lighting & Shadows Design

## Problem
Scene3D is rigid (always loads skybox + terrain). P3 requires lighting, shadows, camera roll,
and streetlights. Future scenes (blackhole) need different skybox, no terrain, custom lighting.

## Solution
1. Make Scene3D configurable via Scene3DConfig struct
2. Add camera roll (Q/E keys)
3. Build a LightingSystem (directional sun + spot streetlights with shadow maps)
4. Update all meshes with normals, create lit shaders
5. Add shadow mapping pipeline (depth pass per light + main pass)

## Architecture

### Scene3DConfig
Scenes pass a config struct controlling which components are active:
- useSkybox (bool) + skyboxPath (string) — configurable skybox folder
- useTerrain (bool) + terrainGenerator — optional terrain
- useLighting (bool) — enables the lighting system
- Camera position, FOV, near/far planes

### Camera Roll
- Q/E keys rotate the up vector around the look direction
- New `roll` member alongside yaw/pitch

### Lighting System
- DirectionalLight: direction, color, intensity (the sun)
- SpotLight: position, direction, color, intensity, cutOff, outerCutOff (streetlights)
- LightingSystem: manages all lights + shadow map FBOs + depth textures
- Blinn-Phong shading model in fragment shaders

### Shadow Mapping
- Depth-only render pass per shadow-casting light
- Directional light: orthographic projection shadow map
- Spot lights: perspective projection shadow maps
- Main pass samples shadow maps to determine if fragments are in shadow
- New Scene3D hook: OnRenderGeometry(shader) — draws meshes without shader setup

### Vertex Data Change
All lit meshes: position(3) + normal(3) + UV(2) = 8 floats per vertex
Skybox unchanged (unlit, uses cubemap sampling)

### Render Pipeline (when lighting enabled)
1. Shadow passes: for each shadow-casting light, render geometry into depth FBO
2. Main pass: render skybox (unlit), then terrain + objects with lit shader + shadow maps

### Files
- Create: include/lighting/light.hpp, lighting_system.hpp
- Create: src/lighting/lighting_system.cpp
- Create: resources/shaders/lit.vs, lit.fs, shadow.vs, shadow.fs
- Modify: scene3d.hpp/cpp (Scene3DConfig, lighting integration, OnRenderGeometry)
- Modify: camera.hpp/cpp (add roll)
- Modify: terrain, road, static_object (add normals)
- Create: p3_scene.hpp/cpp
