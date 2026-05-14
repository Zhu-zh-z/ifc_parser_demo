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

class IfcExtractor : public IIfcExtractor {
public:
    ExtractResult extract(const IfcInput& input) override;
};

std::string formatExtractionLogLine(const IfcInstance& instance);
