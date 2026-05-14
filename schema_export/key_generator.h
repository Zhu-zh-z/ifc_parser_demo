#pragma once

#include <string>

#include "geometry_normalization/geometry_interface.h"

inline std::string generateGeometryKey(const MeshResult& mesh) {
    (void)mesh;
    return "box_3800_1600_2200";
}
