#pragma once

#include <string>

struct IfcInput {
    std::string sourcePath;
    std::string content;
};

class IIfcReader {
public:
    virtual ~IIfcReader() = default;
    virtual IfcInput read(const std::string& path) = 0;
};

class IfcFileReader : public IIfcReader {
public:
    IfcInput read(const std::string& path) override;
};
