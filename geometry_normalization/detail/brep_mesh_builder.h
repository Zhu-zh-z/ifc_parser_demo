#pragma once

#include "geometry_normalization/geometry_interface.h"
#include "ifc_extractor/instance.h"

namespace geometry_normalization::detail {

MeshResult buildBrepStubMesh(const IfcInstance& instance);

}
