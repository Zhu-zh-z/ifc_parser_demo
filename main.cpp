#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "geometry_normalization/geometry_interface.h"
#include "ifc_extractor/extractor.h"
#include "io/ifc_reader.h"
#include "schema_export/batch_hint.h"
#include "schema_export/json_exporter.h"
#include "schema_export/key_generator.h"

namespace {

void printUsage() {
    std::cerr << "Usage: ifc-parse <input.ifc> -o <output.json>\n";
}

} 

int main(int argc, char** argv) {
    if (argc != 4 || std::string(argv[2]) != "-o") {
        printUsage();
        return 1;
    }

    const std::string inputPath = argv[1];
    const std::string outputPath = argv[3];

    try {
        // io
        IfcFileReader reader;
        const IfcInput input = reader.read(inputPath);

        // ifc_extractor
        IfcExtractor extractor;
        ExtractResult extracted = extractor.extract(input);
        if (extracted.instances.empty()) {
            throw std::runtime_error("Extractor returned no instances.");
        }

        // geometry_normalization
        std::unique_ptr<IGeometryNormalizer> normalizer = createGeometryMyImpl();
        const MeshResult mesh = normalizer->buildMesh(extracted.instances.front());

        // schema keys/hints + JSON export
        const std::string geometryKey = generateGeometryKey(mesh);
        const IfcInstance& primaryInstance = extracted.instances.front();

        BatchHint hint;
        hint.geometryKey = geometryKey;
        hint.canInstance = !mesh.positions.empty()
            && !mesh.indices.empty()
            && (primaryInstance.sourceType == "IfcFacetedBrep"
                || primaryInstance.sourceType == "IfcTriangulatedFaceSet");

        ExportDocument doc;
        doc.geometryId = primaryInstance.geometryId.empty()
            ? "geom_box_001"
            : primaryInstance.geometryId;
        doc.geometryKey = geometryKey;
        doc.mesh = mesh;
        doc.instance = primaryInstance;
        doc.batchHint = hint;

        JsonExporter exporter;
        exporter.exportToFile(doc, outputPath);

        std::cout << "Wrote JSON: " << outputPath << "\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "ifc-parse error: " << ex.what() << "\n";
        return 1;
    }
}
