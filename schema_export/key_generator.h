#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

#include "geometry_normalization/geometry_interface.h"

inline std::string generateGeometryKey(const MeshResult& mesh) {
    if (mesh.positions.empty()) {
        return "gk_empty";
    }

    constexpr double kQuantizeScale = 100000.0;
    constexpr std::uint64_t kFnvOffset = 1469598103934665603ULL;
    constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

    const auto quantize = [](float value) -> std::int64_t {
        return static_cast<std::int64_t>(std::llround(static_cast<double>(value) * kQuantizeScale));
    };

    const auto mixByte = [&](std::uint64_t& state, std::uint8_t byte) {
        state ^= static_cast<std::uint64_t>(byte);
        state *= kFnvPrime;
    };

    const auto mixUInt64 = [&](std::uint64_t& state, std::uint64_t value) {
        for (int i = 0; i < 8; ++i) {
            const std::uint8_t byte = static_cast<std::uint8_t>((value >> (i * 8)) & 0xFFU);
            mixByte(state, byte);
        }
    };

    const auto mixInt64 = [&](std::uint64_t& state, std::int64_t value) {
        mixUInt64(state, static_cast<std::uint64_t>(value));
    };

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

    for (const Vec3& position : mesh.positions) {
        minX = std::min(minX, position.x);
        minY = std::min(minY, position.y);
        minZ = std::min(minZ, position.z);
        maxX = std::max(maxX, position.x);
        maxY = std::max(maxY, position.y);
        maxZ = std::max(maxZ, position.z);
    }

    const float dimX = maxX - minX;
    const float dimY = maxY - minY;
    const float dimZ = maxZ - minZ;

    std::uint64_t hashState = kFnvOffset;

    mixInt64(hashState, quantize(dimX));
    mixInt64(hashState, quantize(dimY));
    mixInt64(hashState, quantize(dimZ));
    mixUInt64(hashState, static_cast<std::uint64_t>(mesh.positions.size()));
    mixUInt64(hashState, static_cast<std::uint64_t>(mesh.normals.size()));
    mixUInt64(hashState, static_cast<std::uint64_t>(mesh.uvs.size()));
    mixUInt64(hashState, static_cast<std::uint64_t>(mesh.indices.size()));

    // Translation canonicalization happens before key generation. This pass does
    // not canonicalize vertex/index ordering across different source paths.
    for (const Vec3& position : mesh.positions) {
        mixInt64(hashState, quantize(position.x));
        mixInt64(hashState, quantize(position.y));
        mixInt64(hashState, quantize(position.z));
    }

    for (const Vec3& normal : mesh.normals) {
        mixInt64(hashState, quantize(normal.x));
        mixInt64(hashState, quantize(normal.y));
        mixInt64(hashState, quantize(normal.z));
    }

    for (const Vec2& uv : mesh.uvs) {
        mixInt64(hashState, quantize(uv.x));
        mixInt64(hashState, quantize(uv.y));
    }

    for (unsigned int index : mesh.indices) {
        mixUInt64(hashState, static_cast<std::uint64_t>(index));
    }

    std::ostringstream out;
    out << "gk_"
        << std::hex << std::setfill('0') << std::setw(16)
        << hashState;
    return out.str();
}
