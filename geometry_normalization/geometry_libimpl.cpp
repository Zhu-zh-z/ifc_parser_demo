#include "geometry_normalization/geometry_interface.h"

#include <memory>

namespace {

class StubGeometryNormalizer final : public IGeometryNormalizer {
public:
    MeshResult buildMesh(const IfcInstance& instance) override {
        (void)instance;
        return MeshResult{};
    }
};

} 

std::unique_ptr<IGeometryNormalizer> createGeometryLibImpl() {
    return std::make_unique<StubGeometryNormalizer>();
}
