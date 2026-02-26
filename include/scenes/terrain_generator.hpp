#pragma once

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
