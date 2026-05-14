#include "geometry_normalization/detail/brep_mesh_builder.h"

#include <array>
#include <cstddef>
#include <cmath>
#include <numeric>
#include <vector>

#include "geometry_normalization/detail/geometry_math.h"
#include "geometry_normalization/detail/mesh_builder.h"
#include "utils/epsilon.h"
#include "utils/vec2.h"

namespace geometry_normalization::detail {

namespace {

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
    return GeometryMath::lengthSquared(GeometryMath::cross(a, b)) <= kEpsilon * kEpsilon;
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

Vec3 computeCentroid(const std::vector<Vec3>& points) {
    Vec3 centroid{};
    if (points.empty()) {
        return centroid;
    }

    for (const Vec3& point : points) {
        centroid = GeometryMath::add(centroid, point);
    }
    return GeometryMath::scale(centroid, 1.0f / static_cast<float>(points.size()));
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

}

MeshResult buildBrepMesh(const IfcInstance& instance, BrepBuildStats* stats) {
    MeshBuilder builder;
    BrepBuildStats localStats;
    localStats.inputFaceCount = instance.faces.size();

    std::vector<std::vector<Vec3>> cleanedFaces;
    cleanedFaces.reserve(instance.faces.size());

    Vec3 shellCenterAccumulator{};
    size_t shellCenterPointCount = 0;
    for (const IfcFace& face : instance.faces) {
        localStats.inputFaceVertexCount += face.vertices.size();
        std::vector<Vec3> cleaned = cleanPolygon(face.vertices);
        if (cleaned.size() < 3) {
            ++localStats.skippedFaceCount;
            continue;
        }

        ++localStats.cleanedFaceCount;
        localStats.cleanedFaceVertexCount += cleaned.size();
        shellCenterAccumulator = GeometryMath::add(shellCenterAccumulator, computeCentroid(cleaned));
        ++shellCenterPointCount;
        cleanedFaces.push_back(std::move(cleaned));
    }

    Vec3 shellCenter{};
    if (shellCenterPointCount > 0) {
        shellCenter = GeometryMath::scale(shellCenterAccumulator, 1.0f / static_cast<float>(shellCenterPointCount));
    }

    std::vector<std::array<size_t, 3>> triangles;
    for (const std::vector<Vec3>& cleaned : cleanedFaces) {
        if (!triangulateFace(cleaned, triangles)) {
            ++localStats.skippedFaceCount;
            continue;
        }

        const Vec3 faceNormal = computeFaceNormalNewell(cleaned);
        const Vec3 faceCenter = computeCentroid(cleaned);
        const Vec3 outwardHint = GeometryMath::sub(faceCenter, shellCenter);
        const bool flipWinding = GeometryMath::dot(faceNormal, outwardHint) < 0.0f;

        ++localStats.triangulatedFaceCount;
        for (const auto& tri : triangles) {
            if (flipWinding) {
                builder.addTriangle(cleaned[tri[0]], cleaned[tri[2]], cleaned[tri[1]]);
            } else {
                builder.addTriangle(cleaned[tri[0]], cleaned[tri[1]], cleaned[tri[2]]);
            }
        }
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
