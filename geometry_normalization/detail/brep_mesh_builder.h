#pragma once

#include <cstddef>

#include "geometry_normalization/geometry_interface.h"
#include "ifc_extractor/instance.h"

namespace geometry_normalization::detail {

struct BrepBuildStats {
    size_t inputFaceCount = 0;
    size_t inputFaceVertexCount = 0;
    size_t cleanedFaceCount = 0;
    size_t cleanedFaceVertexCount = 0;
    size_t triangulatedFaceCount = 0;
    size_t skippedFaceCount = 0;
    size_t weldedVertexCount = 0;
    size_t outputIndexCount = 0;
    size_t degenerateTriangleCount = 0;
};

MeshResult buildBrepMesh(const IfcInstance& instance, BrepBuildStats* stats);

}
