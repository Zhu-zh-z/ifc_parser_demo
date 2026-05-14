#pragma once

#include "geometry_normalization/geometry_interface.h"

namespace geometry_normalization::detail {

class BuiltinGeometryNormalizer : public IGeometryNormalizer {
public:
    MeshResult buildMesh(const IfcInstance& instance) override;
};

}
