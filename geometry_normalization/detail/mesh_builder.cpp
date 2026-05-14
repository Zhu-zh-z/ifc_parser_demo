#include "geometry_normalization/detail/mesh_builder.h"

#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "geometry_normalization/detail/geometry_math.h"
#include "utils/epsilon.h"

namespace geometry_normalization::detail {

namespace {

size_t hashCombine(size_t seed, size_t value) {
    return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U));
}

struct FlatVertexKey {
    unsigned int positionIndex = 0;
    long long nx = 0;
    long long ny = 0;
    long long nz = 0;

    bool operator==(const FlatVertexKey& other) const {
        return positionIndex == other.positionIndex &&
               nx == other.nx &&
               ny == other.ny &&
               nz == other.nz;
    }
};

struct FlatVertexKeyHash {
    size_t operator()(const FlatVertexKey& key) const {
        size_t seed = std::hash<unsigned int>{}(key.positionIndex);
        seed = hashCombine(seed, std::hash<long long>{}(key.nx));
        seed = hashCombine(seed, std::hash<long long>{}(key.ny));
        seed = hashCombine(seed, std::hash<long long>{}(key.nz));
        return seed;
    }
};

FlatVertexKey makeFlatVertexKey(unsigned int positionIndex, const Vec3& faceNormal) {
    // Quantize face normal so adjacent coplanar triangles share hard-edge vertices.
    constexpr double kNormalQuantization = 1'000'000.0;
    return FlatVertexKey{
        positionIndex,
        static_cast<long long>(std::llround(static_cast<double>(faceNormal.x) * kNormalQuantization)),
        static_cast<long long>(std::llround(static_cast<double>(faceNormal.y) * kNormalQuantization)),
        static_cast<long long>(std::llround(static_cast<double>(faceNormal.z) * kNormalQuantization)),
    };
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
    const std::vector<Vec3>& sourcePositions = builder.positions();
    const std::vector<unsigned int>& sourceIndices = builder.indices();

    mesh.positions.clear();
    mesh.normals.clear();
    mesh.indices.clear();
    mesh.uvs.clear();

    mesh.positions.reserve(sourceIndices.size());
    mesh.normals.reserve(sourceIndices.size());
    mesh.indices.reserve(sourceIndices.size());

    std::unordered_map<FlatVertexKey, unsigned int, FlatVertexKeyHash> flatVertexToIndex;
    flatVertexToIndex.reserve(sourceIndices.size());

    const auto getOrAddFlatVertex = [&](unsigned int sourceIndex, const Vec3& faceNormal) -> unsigned int {
        const FlatVertexKey key = makeFlatVertexKey(sourceIndex, faceNormal);
        const auto existing = flatVertexToIndex.find(key);
        if (existing != flatVertexToIndex.end()) {
            return existing->second;
        }

        const unsigned int outIndex = static_cast<unsigned int>(mesh.positions.size());
        mesh.positions.push_back(sourcePositions[sourceIndex]);
        mesh.normals.push_back(faceNormal);
        flatVertexToIndex.emplace(key, outIndex);
        return outIndex;
    };

    for (size_t i = 0; i + 2 < sourceIndices.size(); i += 3) {
        const unsigned int ia = sourceIndices[i];
        const unsigned int ib = sourceIndices[i + 1];
        const unsigned int ic = sourceIndices[i + 2];

        const Vec3& a = sourcePositions[ia];
        const Vec3& b = sourcePositions[ib];
        const Vec3& c = sourcePositions[ic];

        const Vec3 faceNormalRaw = GeometryMath::cross(GeometryMath::sub(b, a), GeometryMath::sub(c, a));
        if (GeometryMath::lengthSquared(faceNormalRaw) <= kEpsilon * kEpsilon) {
            continue;
        }

        const Vec3 faceNormal = GeometryMath::normalizeOrZero(faceNormalRaw);
        const unsigned int oa = getOrAddFlatVertex(ia, faceNormal);
        const unsigned int ob = getOrAddFlatVertex(ib, faceNormal);
        const unsigned int oc = getOrAddFlatVertex(ic, faceNormal);

        mesh.indices.push_back(oa);
        mesh.indices.push_back(ob);
        mesh.indices.push_back(oc);
    }

    return mesh;
}

}
