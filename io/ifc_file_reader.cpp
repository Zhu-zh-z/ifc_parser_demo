#include "io/ifc_reader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

IfcInput IfcFileReader::read(const std::string& path) {
    std::ifstream input(path, std::ios::in | std::ios::binary);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open IFC file: " + path);
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (!input.good() && !input.eof()) {
        throw std::runtime_error("Failed while reading IFC file: " + path);
    }

    return IfcInput{path, buffer.str()};
}
