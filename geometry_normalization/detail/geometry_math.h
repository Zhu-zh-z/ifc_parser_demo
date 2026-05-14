#pragma once

#include <cmath>

#include "utils/epsilon.h"
#include "utils/vec2.h"
#include "utils/vec3.h"

namespace geometry_normalization::detail::GeometryMath {

inline Vec3 add(const Vec3& a, const Vec3& b) {
    return Vec3{a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vec3 sub(const Vec3& a, const Vec3& b) {
    return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vec3 scale(const Vec3& v, float s) {
    return Vec3{v.x * s, v.y * s, v.z * s};
}

inline float dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

inline float lengthSquared(const Vec3& v) {
    return dot(v, v);
}

inline float length(const Vec3& v) {
    return std::sqrt(lengthSquared(v));
}

inline Vec3 normalizeOrZero(const Vec3& v) {
    const float len = length(v);
    if (len <= kEpsilon) {
        return Vec3{};
    }
    return scale(v, 1.0f / len);
}

inline bool approxEqual(float a, float b, float epsilon = kEpsilon) {
    return std::fabs(a - b) <= epsilon;
}

inline bool approxEqual(const Vec3& a, const Vec3& b, float epsilon = kEpsilon) {
    return approxEqual(a.x, b.x, epsilon) &&
           approxEqual(a.y, b.y, epsilon) &&
           approxEqual(a.z, b.z, epsilon);
}

inline float triangleArea2(const Vec2& a, const Vec2& b, const Vec2& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

inline float triangleArea3D2(const Vec3& a, const Vec3& b, const Vec3& c) {
    const Vec3 ab = sub(b, a);
    const Vec3 ac = sub(c, a);
    return length(cross(ab, ac));
}

inline bool isDegenerateTriangle(const Vec3& a, const Vec3& b, const Vec3& c) {
    return triangleArea3D2(a, b, c) <= kEpsilon;
}

}
