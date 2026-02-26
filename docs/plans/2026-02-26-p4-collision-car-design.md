# P4 — Drivable Car with AABB Collision

## Overview
P4 reuses the P3 lit environment (buildings, trees, lamp poles, road, full lighting with shadows) and adds a WASD-controlled car cube that collides with static objects.

## Car
- Rendered as a cube with a distinct texture
- State: `position` (vec3), `yaw` (float, rotation around Y), `speed` (float)
- Controls: W/S = forward/backward along facing direction, A/D = rotate left/right
- Sits on ground plane (y=0)
- 3rd-person camera locked behind car: offset `(-sin(yaw)*dist, height, -cos(yaw)*dist)`
- Mouse-look disabled — camera follows car orientation

## Collision System
- AABB (Axis-Aligned Bounding Box) overlap detection
- `struct AABB { vec3 min, max; }` with `Overlaps(const AABB& other)` test
- Helper: `AABBFromObject(ObjectInstance)` — computes world AABB accounting for the +0.5y render shift
- Car AABB computed from position + small scale (1.5 x 1.0 x 1.5)

## Collision Response
- Per-axis resolution: try X movement, check overlap with all objects, revert if colliding. Then try Z.
- This gives natural wall-sliding (approach at angle, slide along surface).
- Visual feedback: on collision, flash car red briefly via a `collisionTimer` that decays each frame.

## Scene Setup
- P4Scene extends Scene3D with `useLighting=true`
- Copies P3's object setup (buildings, trees, lamp poles) and light setup (sun, spots, points)
- Adds drivable car rendered in both lit pass and shadow pass

## New Files
- `include/collision/aabb.hpp` — AABB struct, overlap test, ObjectInstance helper (header-only)

## Modified Files
- `include/scenes/p4_scene.hpp` — extend Scene3D, car state, object list
- `src/scenes/p4_scene.cpp` — full scene implementation

## Texture Unit Layout (unchanged)
| Unit | Usage |
|------|-------|
| 0 | Material texture |
| 1 | Sun shadow map |
| 2-5 | Spot shadow maps |
| 6-8 | Point cubemap shadows |
