#pragma once

#include <array>

#include "geometry_normalization/geometry_interface.h"
#include "schema_export/detail/bounding_box.h"

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

    const MeshBoundingBox boundingBox = computeMeshBoundingBox(result.mesh.positions);
    result.localOffset = boundingBox.center();

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
