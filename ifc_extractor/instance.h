#pragma once

#include <string>
#include <vector>

#include "ifc_extractor/face.h"
#include "utils/vec3.h"

struct IfcInstance {
    std::string instanceId;
    std::string geometryId;
    std::vector<IfcFace> faces;
    std::vector<Vec3> vertices;
    std::vector<unsigned int> triangleIndices;
    std::string sourceType;
};
