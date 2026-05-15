#pragma once

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#include "geometry_normalization/geometry_interface.h"
#include "schema_export/detail/bounding_box.h"

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

    const Vec3 dimensions = computeMeshBoundingBox(mesh.positions).dimensions();

    std::uint64_t hashState = kFnvOffset;

    mixInt64(hashState, quantize(dimensions.x));
    mixInt64(hashState, quantize(dimensions.y));
    mixInt64(hashState, quantize(dimensions.z));
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
