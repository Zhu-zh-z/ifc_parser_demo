#pragma once

#include <memory>
#include <vector>

#include "ifc_extractor/instance.h"
#include "utils/vec2.h"
#include "utils/vec3.h"

struct MeshResult {
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;
    std::vector<unsigned int> indices;
};

class IGeometryNormalizer {
public:
    virtual ~IGeometryNormalizer() = default;
    virtual MeshResult buildMesh(const IfcInstance& instance) = 0;
};

std::unique_ptr<IGeometryNormalizer> createGeometryMyImpl();
std::unique_ptr<IGeometryNormalizer> createGeometryLibImpl();
