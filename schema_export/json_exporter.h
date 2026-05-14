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
        out << "        \"positions\": [],\n";
        out << "        \"normals\": [],\n";
        out << "        \"uvs\": [],\n";
        out << "        \"indices\": []\n";
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
};
