#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "geometry_normalization/geometry_interface.h"
#include "ifc_extractor/instance.h"

namespace geometry_normalization::detail {

struct TessellatedBuildStats {
    size_t inputVertexCount = 0;
    size_t inputIndexCount = 0;
    size_t weldedVertexCount = 0;
    size_t outputIndexCount = 0;
    size_t invalidIndexCount = 0;
    size_t degenerateTriangleCount = 0;
    std::vector<std::array<unsigned int, 3>> sampleSourceTriangles;
    std::vector<std::array<unsigned int, 3>> sampleOutputTriangles;
};

MeshResult buildTessellatedMesh(const IfcInstance& instance, TessellatedBuildStats* stats);

}
