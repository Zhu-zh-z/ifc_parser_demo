#include "geometry_normalization/detail/builtin_geometry_normalizer.h"

#include <iostream>
#include <stdexcept>

#include "geometry_normalization/detail/brep_mesh_builder.h"
#include "geometry_normalization/detail/tessellated_mesh_builder.h"

namespace geometry_normalization::detail {

MeshResult BuiltinGeometryNormalizer::buildMesh(const IfcInstance& instance) {
    if (instance.sourceType == "IfcTriangulatedFaceSet") {
        TessellatedBuildStats stats;
        MeshResult mesh = buildTessellatedMesh(instance, &stats);

        std::cout
            << "normalization source=IfcTriangulatedFaceSet"
            << " inputVertices=" << stats.inputVertexCount
            << " inputIndices=" << stats.inputIndexCount
            << " weldedVertices=" << stats.weldedVertexCount
            << " outputIndices=" << stats.outputIndexCount
            << " degenerateTriangles=" << stats.degenerateTriangleCount
            << " invalidIndices=" << stats.invalidIndexCount
            << "\n";

        for (size_t i = 0; i < stats.sampleSourceTriangles.size(); ++i) {
            const auto& source = stats.sampleSourceTriangles[i];
            std::cout
                << "normalization sample source=("
                << source[0] << "," << source[1] << "," << source[2] << ")";
            if (i < stats.sampleOutputTriangles.size()) {
                const auto& output = stats.sampleOutputTriangles[i];
                std::cout
                    << " output=("
                    << output[0] << "," << output[1] << "," << output[2] << ")";
            }
            std::cout << "\n";
        }
        return mesh;
    }

    if (instance.sourceType == "IfcFacetedBrep") {
        MeshResult mesh = buildBrepStubMesh(instance);
        std::cout << "normalization source=IfcFacetedBrep status=notImplemented outputIndices=0\n";
        return mesh;
    }

    throw std::runtime_error("Unsupported geometry sourceType for normalization: " + instance.sourceType);
}

}
