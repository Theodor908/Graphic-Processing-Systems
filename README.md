## Purpose of the project

The purpose of this project is to acquire skills related to graphics processing systems using the OpenGL library alongside C++. The goal is to design the basic graphical structures required to build realistic physics simulations, game engines, and other applications in this vast domain.

For the template, I used the [opengl-imgui-cmake-template](https://github.com/dcronqvist/opengl-imgui-cmake-template) by [dcronqvist](https://github.com/dcronqvist).
## Structure of the project

The project is divided into six scenes, each of which loads and unloads its resources into the main window. Every scene corresponds to a different level of difficulty in implementing various graphics concepts.

The first five scenes reuse content from previous ones, starting with the Skybox scene.

The later Raytracing and Black Hole scenes are experimental and were implemented by choice rather than being required by the project.

# First scene (Skybox)

The first scene contains a skybox (with a sunrise texture), terrain (ground texture), and a camera movement system.
<img width="864" height="639" alt="Captură de ecran 2026-03-11 164229" src="https://github.com/user-attachments/assets/6f68b962-9ae2-4634-97bf-e435640142c8" />

# Second scene (Street)

This scene contains an elliptical road, side trees, and buildings, all textured.
<img width="836" height="643" alt="Captură de ecran 2026-03-11 164258" src="https://github.com/user-attachments/assets/7717d6e9-bed1-4e6c-8e24-e611ef13c1de" />

# Third scene (Lighting)

This scene builds on top of the first two and adds a more complex lighting system with an ambient light source and specialized light sources for street lamps (with metallic textures).
<img width="803" height="627" alt="Captură de ecran 2026-03-11 164328" src="https://github.com/user-attachments/assets/c0d2f7d1-303d-4056-92bf-84ac46c1e42a" />

# Fourth scene (Collisions)

This scene builds on the first three and introduces a player represented by a box resembling a car. The player can move along the terrain and the road and detect collisions with other objects using AABB (Axis-Aligned Bounding Boxes).
<img width="799" height="628" alt="Captură de ecran 2026-03-11 164432" src="https://github.com/user-attachments/assets/ecc4c088-b1bd-472b-a6a3-c94c0327d339" />

# Fifth scene (Random and AI Cars)

This scene builds on the first four and adds randomly moving cubes on the terrain, as well as two cars that move exclusively on the road along a predetermined path.
<img width="800" height="631" alt="Captură de ecran 2026-03-11 164514" src="https://github.com/user-attachments/assets/5eb67fb4-c7d0-4792-87f9-b1754641ab92" />

# Black hole scene and ray tracing scene (Raytracing; Blackhole)

These scenes are experimental in nature. I struggled with implementing ray tracing and ultimately decided not to include the results here.

The black hole simulation was challenging, but I managed to implement it, although the solution is not optimized for a high frame rate. The black hole is orbited by a white dwarf whose outer layers are gradually consumed by the black hole. The gravitational lensing effect is my favorite feature of this simulation.
<img width="799" height="631" alt="Captură de ecran 2026-03-11 164832" src="https://github.com/user-attachments/assets/bf3a583e-451b-48f7-985a-fa9e8d893a46" />
<img width="885" height="631" alt="Captură de ecran 2026-03-11 164744" src="https://github.com/user-attachments/assets/e76f0783-4531-40ac-a4cd-9847aaa83d6c" />

