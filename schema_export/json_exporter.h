#pragma once

#include <array>
#include <fstream>
#include <stdexcept>
#include <string>

#include "geometry_normalization/geometry_interface.h"
#include "ifc_extractor/instance.h"
#include "schema_export/batch_hint.h"
#include "schema_export/canonical_mesh.h"

struct ExportDocument {
    std::string geometryId;
    std::string geometryKey;
    MeshResult mesh;
    Vec3 localOffset;
    IfcInstance instance;
    BatchHint batchHint;
};

class JsonExporter {
public:
    void exportToFile(const ExportDocument& doc, const std::string& outputPath) {
        std::ofstream out(outputPath, std::ios::out | std::ios::trunc);
        if (!out.is_open()) {
            throw std::runtime_error("Failed to open output file: " + outputPath);
        }

        out << "{\n";
        out << "  \"geometries\": [\n";
        out << "    {\n";
        out << "      \"geometryId\": \"" << doc.geometryId << "\",\n";
        out << "      \"geometryKey\": \"" << doc.geometryKey << "\",\n";
        out << "      \"mesh\": {\n";
        out << "        \"positions\": ";
        writeVec3Array(out, doc.mesh.positions);
        out << ",\n";
        out << "        \"normals\": ";
        writeVec3Array(out, doc.mesh.normals);
        out << ",\n";
        out << "        \"uvs\": ";
        writeVec2Array(out, doc.mesh.uvs);
        out << ",\n";
        out << "        \"indices\": ";
        writeUIntArray(out, doc.mesh.indices);
        out << "\n";
        out << "      }\n";
        out << "    }\n";
        out << "  ],\n";
        out << "  \"instances\": [\n";
        out << "    {\n";
        out << "      \"instanceId\": \"" << doc.instance.instanceId << "\",\n";
        out << "      \"geometryId\": \"" << doc.geometryId << "\",\n";
        // Current pass only fills local-offset translation in transform.
        out << "      \"transform\": ";
        writeFloatArray(out, makeTransformMatrixFromLocalOffset(doc.localOffset));
        out << ",\n";
        out << "      \"batchHint\": {\n";
        out << "        \"geometryKey\": \"" << doc.batchHint.geometryKey << "\",\n";
        out << "        \"canInstance\": " << (doc.batchHint.canInstance ? "true" : "false") << "\n";
        out << "      }\n";
        out << "    }\n";
        out << "  ]\n";
        out << "}\n";
    }

private:
    template <typename Container, typename ElementWriter>
    static void writeJsonArray(std::ofstream& out,
                               const Container& values,
                               ElementWriter&& writeElement) {
        out << "[";
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                out << ",";
            }
            writeElement(values[i]);
        }
        out << "]";
    }

    static void writeVec3Array(std::ofstream& out, const std::vector<Vec3>& values) {
        writeJsonArray(out, values, [&](const Vec3& value) {
            out << "[" << value.x << "," << value.y << "," << value.z << "]";
        });
    }

    static void writeVec2Array(std::ofstream& out, const std::vector<Vec2>& values) {
        writeJsonArray(out, values, [&](const Vec2& value) {
            out << "[" << value.x << "," << value.y << "]";
        });
    }

    static void writeUIntArray(std::ofstream& out, const std::vector<unsigned int>& values) {
        writeJsonArray(out, values, [&](unsigned int value) {
            out << value;
        });
    }

    template <size_t N>
    static void writeFloatArray(std::ofstream& out, const std::array<float, N>& values) {
        writeJsonArray(out, values, [&](float value) {
            out << value;
        });
    }
};
