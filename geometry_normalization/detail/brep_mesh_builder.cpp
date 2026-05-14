#include "geometry_normalization/detail/brep_mesh_builder.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <numeric>
#include <queue>
#include <unordered_map>
#include <vector>

#include "geometry_normalization/detail/geometry_math.h"
#include "geometry_normalization/detail/mesh_builder.h"
#include "utils/epsilon.h"
#include "utils/vec2.h"

namespace geometry_normalization::detail {

namespace {

struct QuantizedKey {
    long long x = 0;
    long long y = 0;
    long long z = 0;

    bool operator==(const QuantizedKey& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct QuantizedKeyHash {
    size_t operator()(const QuantizedKey& key) const {
        size_t seed = std::hash<long long>{}(key.x);
        seed ^= std::hash<long long>{}(key.y) + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
        seed ^= std::hash<long long>{}(key.z) + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
        return seed;
    }
};

struct TopologyTriangle {
    unsigned int a = 0;
    unsigned int b = 0;
    unsigned int c = 0;
};

struct EdgeKey {
    unsigned int lo = 0;
    unsigned int hi = 0;

    bool operator==(const EdgeKey& other) const {
        return lo == other.lo && hi == other.hi;
    }
};

struct EdgeKeyHash {
    size_t operator()(const EdgeKey& key) const {
        size_t seed = std::hash<unsigned int>{}(key.lo);
        seed ^= std::hash<unsigned int>{}(key.hi) + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
        return seed;
    }
};

struct EdgeUse {
    size_t triangleIndex = 0;
    int sign = 0;
};

struct NeighborRule {
    size_t triangleIndex = 0;
    bool sameFlip = true;
};

QuantizedKey quantizePosition(const Vec3& v) {
    const double inv = 1.0 / static_cast<double>(kEpsilon);
    return QuantizedKey{
        static_cast<long long>(std::llround(static_cast<double>(v.x) * inv)),
        static_cast<long long>(std::llround(static_cast<double>(v.y) * inv)),
        static_cast<long long>(std::llround(static_cast<double>(v.z) * inv)),
    };
}

EdgeKey makeEdgeKey(unsigned int a, unsigned int b) {
    if (a < b) {
        return EdgeKey{a, b};
    }
    return EdgeKey{b, a};
}

int edgeSignForCanonical(unsigned int from, unsigned int to, const EdgeKey& key) {
    if (from == key.lo && to == key.hi) {
        return 1;
    }
    if (from == key.hi && to == key.lo) {
        return -1;
    }
    return 0;
}

size_t dominantAxis(const Vec3& normal) {
    const float ax = std::fabs(normal.x);
    const float ay = std::fabs(normal.y);
    const float az = std::fabs(normal.z);
    if (ax >= ay && ax >= az) {
        return 0;
    }
    if (ay >= az) {
        return 1;
    }
    return 2;
}

Vec2 projectPoint(const Vec3& p, size_t dropAxis) {
    if (dropAxis == 0) {
        return Vec2{p.y, p.z};
    }
    if (dropAxis == 1) {
        return Vec2{p.x, p.z};
    }
    return Vec2{p.x, p.y};
}

Vec3 computeFaceNormalNewell(const std::vector<Vec3>& polygon) {
    Vec3 normal{};
    for (size_t i = 0; i < polygon.size(); ++i) {
        const Vec3& a = polygon[i];
        const Vec3& b = polygon[(i + 1) % polygon.size()];
        normal.x += (a.y - b.y) * (a.z + b.z);
        normal.y += (a.z - b.z) * (a.x + b.x);
        normal.z += (a.x - b.x) * (a.y + b.y);
    }
    return normal;
}

float polygonSignedArea2D(const std::vector<Vec2>& polygon) {
    float areaTwice = 0.0f;
    for (size_t i = 0; i < polygon.size(); ++i) {
        const Vec2& a = polygon[i];
        const Vec2& b = polygon[(i + 1) % polygon.size()];
        areaTwice += a.x * b.y - b.x * a.y;
    }
    return 0.5f * areaTwice;
}

bool pointInTriangle2D(const Vec2& p, const Vec2& a, const Vec2& b, const Vec2& c, bool polygonIsCcw) {
    const float pab = GeometryMath::triangleArea2(a, b, p);
    const float pbc = GeometryMath::triangleArea2(b, c, p);
    const float pca = GeometryMath::triangleArea2(c, a, p);
    if (polygonIsCcw) {
        return pab >= -kEpsilon && pbc >= -kEpsilon && pca >= -kEpsilon;
    }
    return pab <= kEpsilon && pbc <= kEpsilon && pca <= kEpsilon;
}

bool isCollinear(const Vec3& prev, const Vec3& curr, const Vec3& next) {
    const Vec3 a = GeometryMath::sub(curr, prev);
    const Vec3 b = GeometryMath::sub(next, curr);
    const float lenSqA = GeometryMath::lengthSquared(a);
    const float lenSqB = GeometryMath::lengthSquared(b);
    if (lenSqA <= kEpsilon * kEpsilon || lenSqB <= kEpsilon * kEpsilon) {
        return true;
    }

    const float crossLenSq = GeometryMath::lengthSquared(GeometryMath::cross(a, b));
    return crossLenSq <= (kEpsilon * kEpsilon) * lenSqA * lenSqB;
}

std::vector<Vec3> cleanPolygon(const std::vector<Vec3>& raw) {
    std::vector<Vec3> cleaned;
    cleaned.reserve(raw.size());
    for (const Vec3& v : raw) {
        if (!cleaned.empty() && GeometryMath::approxEqual(cleaned.back(), v)) {
            continue;
        }
        cleaned.push_back(v);
    }

    if (cleaned.size() >= 2 && GeometryMath::approxEqual(cleaned.front(), cleaned.back())) {
        cleaned.pop_back();
    }

    bool removed = true;
    while (removed && cleaned.size() >= 3) {
        removed = false;
        for (size_t i = 0; i < cleaned.size(); ++i) {
            const size_t prev = (i + cleaned.size() - 1) % cleaned.size();
            const size_t next = (i + 1) % cleaned.size();
            if (isCollinear(cleaned[prev], cleaned[i], cleaned[next])) {
                cleaned.erase(cleaned.begin() + static_cast<std::ptrdiff_t>(i));
                removed = true;
                break;
            }
        }
    }

    if (cleaned.size() >= 2 && GeometryMath::approxEqual(cleaned.front(), cleaned.back())) {
        cleaned.pop_back();
    }
    return cleaned;
}

bool triangulateFace(const std::vector<Vec3>& polygon3d, std::vector<std::array<size_t, 3>>& outTriangles) {
    outTriangles.clear();
    if (polygon3d.size() < 3) {
        return false;
    }

    const Vec3 faceNormal = computeFaceNormalNewell(polygon3d);
    if (GeometryMath::lengthSquared(faceNormal) <= kEpsilon * kEpsilon) {
        return false;
    }

    const size_t dropAxis = dominantAxis(faceNormal);
    std::vector<Vec2> polygon2d;
    polygon2d.reserve(polygon3d.size());
    for (const Vec3& p : polygon3d) {
        polygon2d.push_back(projectPoint(p, dropAxis));
    }

    const float area = polygonSignedArea2D(polygon2d);
    if (std::fabs(area) <= kEpsilon) {
        return false;
    }
    const bool polygonIsCcw = area > 0.0f;

    std::vector<size_t> ring(polygon3d.size());
    std::iota(ring.begin(), ring.end(), static_cast<size_t>(0));

    size_t guard = 0;
    const size_t maxGuard = polygon3d.size() * polygon3d.size();
    while (ring.size() > 3 && guard < maxGuard) {
        bool earFound = false;
        for (size_t i = 0; i < ring.size(); ++i) {
            const size_t prevRing = (i + ring.size() - 1) % ring.size();
            const size_t nextRing = (i + 1) % ring.size();
            const size_t iPrev = ring[prevRing];
            const size_t iCurr = ring[i];
            const size_t iNext = ring[nextRing];

            const float orient = GeometryMath::triangleArea2(polygon2d[iPrev], polygon2d[iCurr], polygon2d[iNext]);
            const bool isConvex = polygonIsCcw ? (orient > kEpsilon) : (orient < -kEpsilon);
            if (!isConvex) {
                continue;
            }

            bool containsPoint = false;
            for (size_t j = 0; j < ring.size(); ++j) {
                const size_t candidate = ring[j];
                if (candidate == iPrev || candidate == iCurr || candidate == iNext) {
                    continue;
                }
                if (pointInTriangle2D(
                        polygon2d[candidate],
                        polygon2d[iPrev],
                        polygon2d[iCurr],
                        polygon2d[iNext],
                        polygonIsCcw)) {
                    containsPoint = true;
                    break;
                }
            }
            if (containsPoint) {
                continue;
            }

            outTriangles.push_back({iPrev, iCurr, iNext});
            ring.erase(ring.begin() + static_cast<std::ptrdiff_t>(i));
            earFound = true;
            break;
        }

        if (!earFound) {
            return false;
        }
        ++guard;
    }

    if (ring.size() == 3) {
        outTriangles.push_back({ring[0], ring[1], ring[2]});
        return true;
    }
    return false;
}

bool orientClosedMesh(
    size_t vertexCount,
    const std::vector<TopologyTriangle>& triangles,
    std::vector<bool>* outFlipByTriangle,
    bool* outGlobalFlip) {
    if (outFlipByTriangle == nullptr || outGlobalFlip == nullptr || triangles.empty()) {
        return false;
    }

    std::unordered_map<EdgeKey, std::vector<EdgeUse>, EdgeKeyHash> edgeToUses;
    edgeToUses.reserve(triangles.size() * 3U);

    for (size_t triIndex = 0; triIndex < triangles.size(); ++triIndex) {
        const TopologyTriangle& tri = triangles[triIndex];
        const std::array<std::pair<unsigned int, unsigned int>, 3> edges = {
            std::pair<unsigned int, unsigned int>{tri.a, tri.b},
            std::pair<unsigned int, unsigned int>{tri.b, tri.c},
            std::pair<unsigned int, unsigned int>{tri.c, tri.a},
        };

        for (const auto& edge : edges) {
            if (edge.first == edge.second) {
                return false;
            }
            const EdgeKey key = makeEdgeKey(edge.first, edge.second);
            const int sign = edgeSignForCanonical(edge.first, edge.second, key);
            if (sign == 0) {
                return false;
            }
            edgeToUses[key].push_back(EdgeUse{triIndex, sign});
        }
    }

    const size_t edgeCount = edgeToUses.size();
    if (edgeCount == 0) {
        return false;
    }
    for (const auto& entry : edgeToUses) {
        if (entry.second.size() != 2) {
            return false;
        }
    }

    if (vertexCount < 4U) {
        return false;
    }

    if (static_cast<long long>(vertexCount) - static_cast<long long>(edgeCount)
            + static_cast<long long>(triangles.size())
        != 2LL) {
        return false;
    }

    std::vector<std::vector<NeighborRule>> adjacency(triangles.size());
    for (const auto& entry : edgeToUses) {
        const EdgeUse& first = entry.second[0];
        const EdgeUse& second = entry.second[1];
        const bool sameFlip = first.sign != second.sign;

        adjacency[first.triangleIndex].push_back(NeighborRule{second.triangleIndex, sameFlip});
        adjacency[second.triangleIndex].push_back(NeighborRule{first.triangleIndex, sameFlip});
    }

    outFlipByTriangle->assign(triangles.size(), false);
    std::vector<bool> visited(triangles.size(), false);
    std::queue<size_t> frontier;
    frontier.push(0);
    visited[0] = true;

    size_t visitedCount = 0;
    while (!frontier.empty()) {
        const size_t current = frontier.front();
        frontier.pop();
        ++visitedCount;

        for (const NeighborRule& rule : adjacency[current]) {
            const bool expectedFlip = rule.sameFlip ? (*outFlipByTriangle)[current]
                                                    : !(*outFlipByTriangle)[current];
            if (!visited[rule.triangleIndex]) {
                visited[rule.triangleIndex] = true;
                (*outFlipByTriangle)[rule.triangleIndex] = expectedFlip;
                frontier.push(rule.triangleIndex);
                continue;
            }
            if ((*outFlipByTriangle)[rule.triangleIndex] != expectedFlip) {
                return false;
            }
        }
    }

    if (visitedCount != triangles.size()) {
        return false;
    }

    *outGlobalFlip = false;
    return true;
}

double signedVolume6(const Vec3& a, const Vec3& b, const Vec3& c) {
    const Vec3 cross = GeometryMath::cross(b, c);
    return static_cast<double>(GeometryMath::dot(a, cross));
}

}

MeshResult buildBrepMesh(const IfcInstance& instance, BrepBuildStats* stats) {
    MeshBuilder builder;
    BrepBuildStats localStats;
    localStats.inputFaceCount = instance.faces.size();

    std::vector<std::vector<Vec3>> cleanedFaces;
    cleanedFaces.reserve(instance.faces.size());
    for (const IfcFace& face : instance.faces) {
        localStats.inputFaceVertexCount += face.vertices.size();
        std::vector<Vec3> cleaned = cleanPolygon(face.vertices);
        if (cleaned.size() < 3) {
            ++localStats.skippedFaceCount;
            continue;
        }

        ++localStats.cleanedFaceCount;
        localStats.cleanedFaceVertexCount += cleaned.size();
        cleanedFaces.push_back(std::move(cleaned));
    }

    std::unordered_map<QuantizedKey, unsigned int, QuantizedKeyHash> topologyVertexByKey;
    topologyVertexByKey.reserve(cleanedFaces.size() * 4U);
    std::vector<Vec3> topologyVertices;
    topologyVertices.reserve(cleanedFaces.size() * 4U);
    std::vector<TopologyTriangle> topologyTriangles;
    topologyTriangles.reserve(cleanedFaces.size() * 2U);

    std::vector<std::array<size_t, 3>> triangles;
    for (const std::vector<Vec3>& cleaned : cleanedFaces) {
        if (!triangulateFace(cleaned, triangles)) {
            ++localStats.skippedFaceCount;
            continue;
        }

        const auto getOrAddTopologyVertex = [&](const Vec3& v) -> unsigned int {
            const QuantizedKey key = quantizePosition(v);
            const auto existing = topologyVertexByKey.find(key);
            if (existing != topologyVertexByKey.end()) {
                return existing->second;
            }
            const unsigned int next = static_cast<unsigned int>(topologyVertices.size());
            topologyVertices.push_back(v);
            topologyVertexByKey.emplace(key, next);
            return next;
        };

        ++localStats.triangulatedFaceCount;
        for (const auto& tri : triangles) {
            const Vec3& a = cleaned[tri[0]];
            const Vec3& b = cleaned[tri[1]];
            const Vec3& c = cleaned[tri[2]];
            if (GeometryMath::isDegenerateTriangle(a, b, c)) {
                continue;
            }

            const unsigned int ia = getOrAddTopologyVertex(a);
            const unsigned int ib = getOrAddTopologyVertex(b);
            const unsigned int ic = getOrAddTopologyVertex(c);
            if (ia == ib || ib == ic || ic == ia) {
                continue;
            }

            topologyTriangles.push_back(TopologyTriangle{ia, ib, ic});
        }
    }

    std::vector<bool> flipByTriangle;
    bool globalFlip = false;
    if (!orientClosedMesh(topologyVertices.size(), topologyTriangles, &flipByTriangle, &globalFlip)) {
        MeshResult mesh = finalizeMesh(builder);
        localStats.weldedVertexCount = mesh.positions.size();
        localStats.outputIndexCount = mesh.indices.size();
        localStats.degenerateTriangleCount = builder.degenerateTriangleCount();
        if (stats != nullptr) {
            *stats = localStats;
        }
        return mesh;
    }

    double volume6 = 0.0;
    for (size_t triIndex = 0; triIndex < topologyTriangles.size(); ++triIndex) {
        const TopologyTriangle& tri = topologyTriangles[triIndex];
        unsigned int ia = tri.a;
        unsigned int ib = tri.b;
        unsigned int ic = tri.c;
        if (flipByTriangle[triIndex]) {
            std::swap(ib, ic);
        }
        const Vec3& a = topologyVertices[ia];
        const Vec3& b = topologyVertices[ib];
        const Vec3& c = topologyVertices[ic];
        volume6 += signedVolume6(a, b, c);
    }
    if (volume6 < 0.0) {
        globalFlip = true;
    }

    for (size_t triIndex = 0; triIndex < topologyTriangles.size(); ++triIndex) {
        const TopologyTriangle& tri = topologyTriangles[triIndex];
        unsigned int ia = tri.a;
        unsigned int ib = tri.b;
        unsigned int ic = tri.c;
        if (flipByTriangle[triIndex]) {
            std::swap(ib, ic);
        }
        if (globalFlip) {
            std::swap(ib, ic);
        }

        builder.addTriangle(topologyVertices[ia], topologyVertices[ib], topologyVertices[ic]);
    }

    MeshResult mesh = finalizeMesh(builder);
    localStats.weldedVertexCount = mesh.positions.size();
    localStats.outputIndexCount = mesh.indices.size();
    localStats.degenerateTriangleCount = builder.degenerateTriangleCount();

    if (stats != nullptr) {
        *stats = localStats;
    }
    return mesh;
}

}
