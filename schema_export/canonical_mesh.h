#pragma once

#include <algorithm>
#include <array>
#include <limits>

#include "geometry_normalization/geometry_interface.h"

struct CanonicalMeshResult {
    MeshResult mesh;
    Vec3 localOffset;
};

inline CanonicalMeshResult canonicalizeMeshForReuse(const MeshResult& sourceMesh) {
    CanonicalMeshResult result;
    result.mesh = sourceMesh;
    result.localOffset = Vec3{};

    if (result.mesh.positions.empty()) {
        return result;
    }

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

    for (const Vec3& position : result.mesh.positions) {
        minX = std::min(minX, position.x);
        minY = std::min(minY, position.y);
        minZ = std::min(minZ, position.z);
        maxX = std::max(maxX, position.x);
        maxY = std::max(maxY, position.y);
        maxZ = std::max(maxZ, position.z);
    }

    result.localOffset.x = (minX + maxX) * 0.5f;
    result.localOffset.y = (minY + maxY) * 0.5f;
    result.localOffset.z = (minZ + maxZ) * 0.5f;

    for (Vec3& position : result.mesh.positions) {
        position.x -= result.localOffset.x;
        position.y -= result.localOffset.y;
        position.z -= result.localOffset.z;
    }

    return result;
}

inline std::array<float, 16> makeTransformMatrixFromLocalOffset(const Vec3& localOffset) {
    return std::array<float, 16>{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        localOffset.x, localOffset.y, localOffset.z, 1.0f,
    };
}
