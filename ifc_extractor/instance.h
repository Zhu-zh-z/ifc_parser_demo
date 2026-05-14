#pragma once

#include <string>
#include <vector>

#include "ifc_extractor/face.h"

struct IfcInstance {
    std::string instanceId;
    std::string geometryId;
    std::vector<IfcFace> faces;
    std::vector<unsigned int> triangleIndices;
    std::string sourceType;
};
