#pragma once

#include <fstream>
#include <stdexcept>
#include <string>

#include "geometry_normalization/geometry_interface.h"
#include "ifc_extractor/instance.h"
#include "schema_export/batch_hint.h"

struct ExportDocument {
    std::string geometryId;
    std::string geometryKey;
    MeshResult mesh;
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
        out << "      \"transform\": [],\n";
        out << "      \"batchHint\": {\n";
        out << "        \"geometryKey\": \"" << doc.batchHint.geometryKey << "\",\n";
        out << "        \"canInstance\": " << (doc.batchHint.canInstance ? "true" : "false") << "\n";
        out << "      }\n";
        out << "    }\n";
        out << "  ]\n";
        out << "}\n";
    }

private:
    static void writeVec3Array(std::ofstream& out, const std::vector<Vec3>& values) {
        out << "[";
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                out << ",";
            }
            out << "[" << values[i].x << "," << values[i].y << "," << values[i].z << "]";
        }
        out << "]";
    }

    static void writeVec2Array(std::ofstream& out, const std::vector<Vec2>& values) {
        out << "[";
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                out << ",";
            }
            out << "[" << values[i].x << "," << values[i].y << "]";
        }
        out << "]";
    }

    static void writeUIntArray(std::ofstream& out, const std::vector<unsigned int>& values) {
        out << "[";
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                out << ",";
            }
            out << values[i];
        }
        out << "]";
    }
};
