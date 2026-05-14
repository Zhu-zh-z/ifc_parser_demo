#include "geometry_normalization/detail/mesh_builder.h"

#include <cmath>
#include <cstdint>

#include "geometry_normalization/detail/geometry_math.h"
#include "utils/epsilon.h"

namespace geometry_normalization::detail {

namespace {

size_t hashCombine(size_t seed, size_t value) {
    return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U));
}

} 

size_t MeshBuilder::QuantizedKeyHash::operator()(const QuantizedKey& key) const {
    size_t seed = std::hash<long long>{}(key.x);
    seed = hashCombine(seed, std::hash<long long>{}(key.y));
    seed = hashCombine(seed, std::hash<long long>{}(key.z));
    return seed;
}

MeshBuilder::QuantizedKey MeshBuilder::quantize(const Vec3& v) const {
    const double inv = 1.0 / static_cast<double>(kEpsilon);
    return QuantizedKey{
        static_cast<long long>(std::llround(static_cast<double>(v.x) * inv)),
        static_cast<long long>(std::llround(static_cast<double>(v.y) * inv)),
        static_cast<long long>(std::llround(static_cast<double>(v.z) * inv)),
    };
}

unsigned int MeshBuilder::getOrAddVertex(const Vec3& v) {
    const QuantizedKey key = quantize(v);
    const auto existing = weldedIndexByKey_.find(key);
    if (existing != weldedIndexByKey_.end()) {
        return existing->second;
    }

    const unsigned int next = static_cast<unsigned int>(positions_.size());
    positions_.push_back(v);
    weldedIndexByKey_.emplace(key, next);
    return next;
}

bool MeshBuilder::addTriangle(const Vec3& a, const Vec3& b, const Vec3& c, TriangleIndices* added) {
    if (GeometryMath::isDegenerateTriangle(a, b, c)) {
        ++degenerateTriangleCount_;
        return false;
    }

    const unsigned int ia = getOrAddVertex(a);
    const unsigned int ib = getOrAddVertex(b);
    const unsigned int ic = getOrAddVertex(c);
    if (ia == ib || ib == ic || ic == ia) {
        ++degenerateTriangleCount_;
        return false;
    }

    indices_.push_back(ia);
    indices_.push_back(ib);
    indices_.push_back(ic);

    if (added != nullptr) {
        added->a = ia;
        added->b = ib;
        added->c = ic;
    }
    return true;
}

MeshResult finalizeMesh(const MeshBuilder& builder) {
    MeshResult mesh;
    mesh.positions = builder.positions();
    mesh.indices = builder.indices();
    mesh.uvs.clear();

    mesh.normals.assign(mesh.positions.size(), Vec3{});
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const unsigned int ia = mesh.indices[i];
        const unsigned int ib = mesh.indices[i + 1];
        const unsigned int ic = mesh.indices[i + 2];

        const Vec3& a = mesh.positions[ia];
        const Vec3& b = mesh.positions[ib];
        const Vec3& c = mesh.positions[ic];

        const Vec3 faceNormal = GeometryMath::cross(GeometryMath::sub(b, a), GeometryMath::sub(c, a));
        if (GeometryMath::lengthSquared(faceNormal) <= kEpsilon * kEpsilon) {
            continue;
        }

        mesh.normals[ia] = GeometryMath::add(mesh.normals[ia], faceNormal);
        mesh.normals[ib] = GeometryMath::add(mesh.normals[ib], faceNormal);
        mesh.normals[ic] = GeometryMath::add(mesh.normals[ic], faceNormal);
    }

    for (Vec3& normal : mesh.normals) {
        normal = GeometryMath::normalizeOrZero(normal);
    }

    return mesh;
}

}
