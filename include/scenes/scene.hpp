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
