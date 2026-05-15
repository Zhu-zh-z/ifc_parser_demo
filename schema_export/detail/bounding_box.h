#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include "utils/vec3.h"

struct MeshBoundingBox {
    Vec3 min;
    Vec3 max;

    Vec3 center() const {
        return Vec3{
            (min.x + max.x) * 0.5f,
            (min.y + max.y) * 0.5f,
            (min.z + max.z) * 0.5f,
        };
    }

    Vec3 dimensions() const {
        return Vec3{
            max.x - min.x,
            max.y - min.y,
            max.z - min.z,
        };
    }
};

inline MeshBoundingBox computeMeshBoundingBox(const std::vector<Vec3>& positions) {
    MeshBoundingBox bounds{
        Vec3{
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
        },
        Vec3{
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest(),
        },
    };

    for (const Vec3& position : positions) {
        bounds.min.x = std::min(bounds.min.x, position.x);
        bounds.min.y = std::min(bounds.min.y, position.y);
        bounds.min.z = std::min(bounds.min.z, position.z);
        bounds.max.x = std::max(bounds.max.x, position.x);
        bounds.max.y = std::max(bounds.max.y, position.y);
        bounds.max.z = std::max(bounds.max.z, position.z);
    }

    return bounds;
}
