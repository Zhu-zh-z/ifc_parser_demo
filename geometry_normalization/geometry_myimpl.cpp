#include "geometry_normalization/geometry_interface.h"
#include "geometry_normalization/detail/builtin_geometry_normalizer.h"

#include <memory>

std::unique_ptr<IGeometryNormalizer> createGeometryMyImpl() {
    return std::make_unique<geometry_normalization::detail::BuiltinGeometryNormalizer>();
}
