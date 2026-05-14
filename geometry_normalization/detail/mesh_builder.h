#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

#include "geometry_normalization/geometry_interface.h"
#include "utils/vec3.h"

namespace geometry_normalization::detail {

struct TriangleIndices {
    unsigned int a = 0;
    unsigned int b = 0;
    unsigned int c = 0;
};

class MeshBuilder {
public:
    bool addTriangle(const Vec3& a, const Vec3& b, const Vec3& c, TriangleIndices* added = nullptr);

    size_t vertexCount() const {
        return positions_.size();
    }

    size_t indexCount() const {
        return indices_.size();
    }

    size_t degenerateTriangleCount() const {
        return degenerateTriangleCount_;
    }

    const std::vector<Vec3>& positions() const {
        return positions_;
    }

    const std::vector<unsigned int>& indices() const {
        return indices_;
    }

private:
    struct QuantizedKey {
        long long x = 0;
        long long y = 0;
        long long z = 0;

        bool operator==(const QuantizedKey& other) const {
            return x == other.x && y == other.y && z == other.z;
        }
    };

    struct QuantizedKeyHash {
        size_t operator()(const QuantizedKey& key) const;
    };

    unsigned int getOrAddVertex(const Vec3& v);
    QuantizedKey quantize(const Vec3& v) const;

    std::vector<Vec3> positions_;
    std::vector<unsigned int> indices_;
    std::unordered_map<QuantizedKey, unsigned int, QuantizedKeyHash> weldedIndexByKey_;
    size_t degenerateTriangleCount_ = 0;
};

MeshResult finalizeMesh(const MeshBuilder& builder);

}
