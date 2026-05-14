#pragma once

#include <string>
#include <vector>

#include "ifc_extractor/instance.h"
#include "io/ifc_reader.h"

struct ExtractResult {
    std::vector<IfcInstance> instances;
};

class IIfcExtractor {
public:
    virtual ~IIfcExtractor() = default;
    virtual ExtractResult extract(const IfcInput& input) = 0;
};

class StubIfcExtractor : public IIfcExtractor {
public:
    ExtractResult extract(const IfcInput& input) override {
        (void)input;
        IfcInstance stub;
        stub.instanceId = "inst_001";
        stub.geometryId = "geom_box_001";
        stub.sourceType = "stub";
        return ExtractResult{{stub}};
    }
};
