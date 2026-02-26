#pragma once
#include <glm/glm.hpp>
#include "scenes/static_object.hpp"

struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    bool Overlaps(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }

    // XZ-only overlap for ground-plane collision (ignores Y axis)
    bool OverlapsXZ(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }
};

// Build AABB from ObjectInstance (matches the render transform:
//   translate(position) * scale(scale) * translate(0, 0.5, 0))
// Result: x in [pos.x - scale.x/2, pos.x + scale.x/2]
//         y in [pos.y, pos.y + scale.y]
//         z in [pos.z - scale.z/2, pos.z + scale.z/2]
inline AABB AABBFromObject(const ObjectInstance& obj) {
    glm::vec3 half(obj.scale.x * 0.5f, 0.0f, obj.scale.z * 0.5f);
    return {
        glm::vec3(obj.position.x - half.x, obj.position.y, obj.position.z - half.z),
        glm::vec3(obj.position.x + half.x, obj.position.y + obj.scale.y, obj.position.z + half.z)
    };
}

// Build AABB for a car-like object centered at position with given half-extents
inline AABB AABBFromCar(const glm::vec3& position, const glm::vec3& halfExtents) {
    return {
        position - halfExtents,
        position + halfExtents
    };
}
