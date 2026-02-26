# P5 — AI Cars + Wandering Cubes

## Overview
P5 builds on P4's lit environment and drivable car. Adds 2 AI cars that orbit the elliptical road and 4-6 wandering cubes that drift around the scene bouncing off buildings. Everything is solid — the player car collides with AI cars and wandering cubes.

## AI Cars (2)
- Follow road center-line: `pos = (37.5*cos(t), roadY, 27.5*sin(t))`
- Starting offsets: 0° and 180° (opposite sides of the loop)
- Yaw from ellipse tangent: `atan2(-37.5*sin(t), 27.5*cos(t))`
- Constant speed (different for each car for variety)
- Rendered as cubes, cast shadows
- Dynamic AABB updated each frame

## Wandering Cubes (4-6)
- Spawn at random XZ positions in the grass area (outside road)
- Random direction on XZ plane, constant speed
- Bounce off static objects: reflect blocked axis on collision (per-axis resolution)
- Small cubes (~1x1x1), distinct texture
- Dynamic AABB updated each frame

## Collision
- Static colliders: buildings, trees, poles (precomputed)
- Dynamic colliders: AI cars + wandering cubes (recomputed each frame)
- Player car checks all colliders (static + dynamic)
- Wandering cubes check static colliders only (for bouncing)
- AI cars don't check collision (they follow a fixed path)

## Files
- Modify: `include/scenes/p5_scene.hpp` — Scene3D subclass with all state
- Modify: `src/scenes/p5_scene.cpp` — full implementation
