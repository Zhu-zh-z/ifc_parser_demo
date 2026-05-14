#include "geometry_normalization/detail/tessellated_mesh_builder.h"

#include <stdexcept>

#include "geometry_normalization/detail/mesh_builder.h"

namespace geometry_normalization::detail {

MeshResult buildTessellatedMesh(const IfcInstance& instance, TessellatedBuildStats* stats) {
    if (instance.vertices.empty()) {
        throw std::runtime_error("IfcTriangulatedFaceSet has no vertices.");
    }

    if (instance.triangleIndices.empty()) {
        throw std::runtime_error("IfcTriangulatedFaceSet has no indices.");
    }

    MeshBuilder builder;
    TessellatedBuildStats localStats;
    localStats.inputVertexCount = instance.vertices.size();
    localStats.inputIndexCount = instance.triangleIndices.size();

    const size_t triangleCount = instance.triangleIndices.size() / 3;
    const size_t trailing = instance.triangleIndices.size() % 3;
    if (trailing != 0) {
        localStats.invalidIndexCount += trailing;
    }

    localStats.sampleSourceTriangles.reserve(3);
    localStats.sampleOutputTriangles.reserve(3);

    for (size_t tri = 0; tri < triangleCount; ++tri) {
        const unsigned int ia = instance.triangleIndices[tri * 3];
        const unsigned int ib = instance.triangleIndices[tri * 3 + 1];
        const unsigned int ic = instance.triangleIndices[tri * 3 + 2];

        if (ia >= instance.vertices.size() || ib >= instance.vertices.size() || ic >= instance.vertices.size()) {
            localStats.invalidIndexCount += 3;
            continue;
        }

        TriangleIndices added{};
        const bool accepted = builder.addTriangle(
            instance.vertices[ia],
            instance.vertices[ib],
            instance.vertices[ic],
            &added);

        if (localStats.sampleSourceTriangles.size() < 3) {
            localStats.sampleSourceTriangles.push_back({ia, ib, ic});
            if (accepted) {
                localStats.sampleOutputTriangles.push_back({added.a, added.b, added.c});
            }
        }
    }

    MeshResult mesh = finalizeMesh(builder);
    localStats.weldedVertexCount = mesh.positions.size();
    localStats.outputIndexCount = mesh.indices.size();
    localStats.degenerateTriangleCount = builder.degenerateTriangleCount();

    if (stats != nullptr) {
        *stats = localStats;
    }
    return mesh;
}

}
