#include "geometry_normalization/geometry_interface.h"

#include <memory>

std::unique_ptr<IGeometryNormalizer> createGeometryMyImpl() {
    return std::make_unique<StubGeometryNormalizer>();
}
