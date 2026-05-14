#pragma once

#include <string>

struct BatchHint {
    std::string geometryKey;
    bool canInstance = false;
    bool hasUv = false;
    bool hasNormal = false;
};
