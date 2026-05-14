#include "ifc_extractor/extractor.h"

#include <charconv>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

struct StepEntity {
    int id = 0;
    std::string type;
    std::vector<std::string_view> args;
};

std::string_view trimView(std::string_view value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }

    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }

    return value.substr(start, end - start);
}

std::string toUpper(std::string_view value) {
    std::string result;
    result.reserve(value.size());
    for (char c : value) {
        result.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return result;
}

std::vector<std::string_view> splitTopLevel(std::string_view value) {
    std::vector<std::string_view> parts;
    int depth = 0;
    bool inString = false;
    size_t segmentStart = 0;

    for (size_t i = 0; i < value.size(); ++i) {
        const char c = value[i];
        if (c == '\'') {
            inString = !inString;
            continue;
        }

        if (!inString) {
            if (c == '(') {
                ++depth;
            } else if (c == ')') {
                --depth;
            } else if (c == ',' && depth == 0) {
                parts.push_back(trimView(value.substr(segmentStart, i - segmentStart)));
                segmentStart = i + 1;
                continue;
            }
        }
    }

    if (segmentStart < value.size()) {
        parts.push_back(trimView(value.substr(segmentStart)));
    }

    return parts;
}

std::string_view stripOuterParens(std::string_view value) {
    const std::string_view trimmed = trimView(value);
    if (trimmed.size() >= 2 && trimmed.front() == '(' && trimmed.back() == ')') {
        return trimmed.substr(1, trimmed.size() - 2);
    }
    return trimmed;
}

bool parseIntToken(std::string_view token, int& result) {
    token = trimView(token);
    if (token.empty()) {
        return false;
    }
    const char* start = token.data();
    const char* end = token.data() + token.size();
    const auto parseResult = std::from_chars(start, end, result);
    return parseResult.ec == std::errc() && parseResult.ptr == end;
}

bool parseDoubleToken(std::string_view token, double& result) {
    token = trimView(token);
    if (token.empty() || token == "$") {
        return false;
    }

    const std::string owned(token);
    char* parseEnd = nullptr;
    result = std::strtod(owned.c_str(), &parseEnd);
    return parseEnd != owned.c_str() && *parseEnd == '\0';
}

std::vector<int> parseRefList(std::string_view value) {
    std::vector<int> refs;
    const std::string_view content = stripOuterParens(value);
    for (std::string_view part : splitTopLevel(content)) {
        const std::string_view token = trimView(part);
        if (token.size() >= 2 && token.front() == '#') {
            int ref = 0;
            if (parseIntToken(token.substr(1), ref)) {
                refs.push_back(ref);
            }
        }
    }
    return refs;
}

std::vector<double> parseDoubleList(std::string_view value) {
    std::vector<double> numbers;
    const std::string_view content = stripOuterParens(value);
    for (std::string_view part : splitTopLevel(content)) {
        double number = 0.0;
        if (parseDoubleToken(part, number)) {
            numbers.push_back(number);
        }
    }
    return numbers;
}

bool tryParseVec3(std::string_view value, Vec3& out) {
    float coords[3] = {};
    int coordCount = 0;
    for (std::string_view part : splitTopLevel(stripOuterParens(value))) {
        if (coordCount >= 3) {
            break;
        }
        double parsed = 0.0;
        if (!parseDoubleToken(part, parsed)) {
            continue;
        }
        coords[coordCount++] = static_cast<float>(parsed);
    }
    if (coordCount < 3) {
        return false;
    }
    out.x = coords[0];
    out.y = coords[1];
    out.z = coords[2];
    return true;
}

void appendPointList3D(std::string_view value, std::vector<Vec3>& vertices) {
    const std::string_view tuples = stripOuterParens(value);
    for (std::string_view tuple : splitTopLevel(tuples)) {
        Vec3 point;
        if (tryParseVec3(tuple, point)) {
            vertices.push_back(point);
        }
    }
}

void appendTriangleIndices(std::string_view value, std::vector<unsigned int>& triangleIndices) {
    for (std::string_view tuple : splitTopLevel(stripOuterParens(value))) {
        for (std::string_view number : splitTopLevel(stripOuterParens(tuple))) {
            int index1Based = 0;
            if (!parseIntToken(number, index1Based) || index1Based <= 0) {
                continue;
            }
            triangleIndices.push_back(static_cast<unsigned int>(index1Based - 1));
        }
    }
}

size_t estimateEntityCount(const std::string& content) {
    const std::string_view view(content);
    size_t start = 0;
    size_t count = 0;
    while (start < view.size()) {
        const size_t end = view.find(';', start);
        const size_t statementEnd = end == std::string_view::npos ? view.size() : end;
        const std::string_view statement = trimView(view.substr(start, statementEnd - start));
        if (!statement.empty() && statement.front() == '#') {
            ++count;
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    return count;
}

std::unordered_map<int, StepEntity> parseEntities(const std::string& content) {
    std::unordered_map<int, StepEntity> entities;
    entities.reserve(estimateEntityCount(content));

    const std::string_view view(content);
    size_t start = 0;
    while (start < view.size()) {
        const size_t end = view.find(';', start);
        const size_t statementEnd = end == std::string_view::npos ? view.size() : end;
        const std::string_view statement = trimView(view.substr(start, statementEnd - start));
        if (statement.empty() || statement.front() != '#') {
            if (end == std::string_view::npos) {
                break;
            }
            start = end + 1;
            continue;
        }

        const size_t equalPos = statement.find('=');
        const size_t lparenPos = statement.find('(', equalPos);
        const size_t rparenPos = statement.rfind(')');
        if (equalPos == std::string_view::npos || lparenPos == std::string_view::npos ||
            rparenPos == std::string_view::npos ||
            rparenPos <= lparenPos) {
            if (end == std::string_view::npos) {
                break;
            }
            start = end + 1;
            continue;
        }

        int id = 0;
        if (!parseIntToken(statement.substr(1, equalPos - 1), id)) {
            if (end == std::string_view::npos) {
                break;
            }
            start = end + 1;
            continue;
        }
        StepEntity entity;
        entity.id = id;
        entity.type = toUpper(trimView(statement.substr(equalPos + 1, lparenPos - equalPos - 1)));
        entity.args = splitTopLevel(statement.substr(lparenPos + 1, rparenPos - lparenPos - 1));
        entities[id] = std::move(entity);

        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }

    return entities;
}

const StepEntity* findEntity(const std::unordered_map<int, StepEntity>& entities, int id) {
    const auto it = entities.find(id);
    if (it == entities.end()) {
        return nullptr;
    }
    return &it->second;
}

Vec3 parseCartesianPoint(const StepEntity& pointEntity) {
    if (pointEntity.type != "IFCCARTESIANPOINT" || pointEntity.args.empty()) {
        throw std::runtime_error("Invalid IFCCARTESIANPOINT entity.");
    }

    const std::vector<double> coords = parseDoubleList(pointEntity.args.front());
    if (coords.size() < 3) {
        throw std::runtime_error("IFCCARTESIANPOINT has fewer than 3 coordinates.");
    }

    Vec3 point;
    point.x = static_cast<float>(coords[0]);
    point.y = static_cast<float>(coords[1]);
    point.z = static_cast<float>(coords[2]);
    return point;
}

std::vector<int> resolveShapeRepresentationRefs(const std::unordered_map<int, StepEntity>& entities, int productShapeId) {
    const StepEntity* productShape = findEntity(entities, productShapeId);
    if (productShape == nullptr || productShape->type != "IFCPRODUCTDEFINITIONSHAPE" || productShape->args.size() < 3) {
        return {};
    }

    std::vector<int> shapeRepresentationIds = parseRefList(productShape->args[2]);
    std::vector<int> geometryIds;
    for (const int shapeRepresentationId : shapeRepresentationIds) {
        const StepEntity* shapeRepresentation = findEntity(entities, shapeRepresentationId);
        if (shapeRepresentation == nullptr || shapeRepresentation->type != "IFCSHAPEREPRESENTATION" ||
            shapeRepresentation->args.size() < 4) {
            continue;
        }

        const std::vector<int> shapeGeometryIds = parseRefList(shapeRepresentation->args[3]);
        geometryIds.insert(geometryIds.end(), shapeGeometryIds.begin(), shapeGeometryIds.end());
    }

    return geometryIds;
}

std::vector<IfcFace> resolveFacetedBrepFaces(const std::unordered_map<int, StepEntity>& entities, int brepId) {
    const StepEntity* brep = findEntity(entities, brepId);
    if (brep == nullptr || brep->type != "IFCFACETEDBREP" || brep->args.empty()) {
        return {};
    }

    const std::vector<int> shellRefs = parseRefList(brep->args[0]);
    if (shellRefs.empty()) {
        return {};
    }

    const StepEntity* shell = findEntity(entities, shellRefs.front());
    if (shell == nullptr || shell->type != "IFCCLOSEDSHELL" || shell->args.empty()) {
        return {};
    }

    std::vector<IfcFace> faces;
    const std::vector<int> faceRefs = parseRefList(shell->args[0]);
    std::unordered_map<int, Vec3> pointCache;
    pointCache.reserve(faceRefs.size() * 4);
    for (const int faceRef : faceRefs) {
        const StepEntity* faceEntity = findEntity(entities, faceRef);
        if (faceEntity == nullptr || faceEntity->type != "IFCFACE" || faceEntity->args.empty()) {
            continue;
        }

        const std::vector<int> faceBoundRefs = parseRefList(faceEntity->args[0]);
        for (const int faceBoundRef : faceBoundRefs) {
            const StepEntity* faceBound = findEntity(entities, faceBoundRef);
            if (faceBound == nullptr || faceBound->type != "IFCFACEOUTERBOUND" || faceBound->args.empty()) {
                continue;
            }

            const std::vector<int> polyLoopRefs = parseRefList(faceBound->args[0]);
            if (polyLoopRefs.empty()) {
                continue;
            }

            const StepEntity* polyLoop = findEntity(entities, polyLoopRefs.front());
            if (polyLoop == nullptr || polyLoop->type != "IFCPOLYLOOP" || polyLoop->args.empty()) {
                continue;
            }

            IfcFace face;
            const std::vector<int> pointRefs = parseRefList(polyLoop->args[0]);
            for (const int pointRef : pointRefs) {
                const auto cached = pointCache.find(pointRef);
                if (cached != pointCache.end()) {
                    face.vertices.push_back(cached->second);
                    continue;
                }

                const StepEntity* pointEntity = findEntity(entities, pointRef);
                if (pointEntity == nullptr) {
                    continue;
                }
                const Vec3 parsed = parseCartesianPoint(*pointEntity);
                pointCache.emplace(pointRef, parsed);
                face.vertices.push_back(parsed);
            }

            if (!face.vertices.empty()) {
                faces.push_back(std::move(face));
            }
        }
    }

    return faces;
}

void resolveTriangulatedFaceSet(const std::unordered_map<int, StepEntity>& entities,
                                int faceSetId,
                                std::vector<Vec3>& vertices,
                                std::vector<unsigned int>& triangleIndices) {
    const StepEntity* faceSet = findEntity(entities, faceSetId);
    if (faceSet == nullptr || faceSet->type != "IFCTRIANGULATEDFACESET" || faceSet->args.size() < 4) {
        return;
    }

    const std::vector<int> pointListRefs = parseRefList(faceSet->args[0]);
    if (!pointListRefs.empty()) {
        const StepEntity* pointList = findEntity(entities, pointListRefs.front());
        if (pointList != nullptr && pointList->type == "IFCCARTESIANPOINTLIST3D" && !pointList->args.empty()) {
            appendPointList3D(pointList->args[0], vertices);
        }
    }

    appendTriangleIndices(faceSet->args[3], triangleIndices);
}

} // namespace

ExtractResult IfcExtractor::extract(const IfcInput& input) {
    const std::unordered_map<int, StepEntity> entities = parseEntities(input.content);
    ExtractResult result;

    for (const auto& [productId, productEntity] : entities) {
        if (productEntity.type != "IFCBUILDINGELEMENTPROXY" || productEntity.args.size() < 7) {
            continue;
        }

        const std::vector<int> productShapeRefs = parseRefList(productEntity.args[6]);
        if (productShapeRefs.empty()) {
            continue;
        }

        IfcInstance instance;
        instance.instanceId = "inst_" + std::to_string(productId);

        const std::vector<int> geometryRefs = resolveShapeRepresentationRefs(entities, productShapeRefs.front());
        for (const int geometryRef : geometryRefs) {
            const StepEntity* geometry = findEntity(entities, geometryRef);
            if (geometry == nullptr) {
                continue;
            }

            if (geometry->type == "IFCFACETEDBREP") {
                instance.sourceType = "IfcFacetedBrep";
                instance.geometryId = "geom_" + std::to_string(geometryRef);
                instance.faces = resolveFacetedBrepFaces(entities, geometryRef);
                break;
            }

            if (geometry->type == "IFCTRIANGULATEDFACESET") {
                instance.sourceType = "IfcTriangulatedFaceSet";
                instance.geometryId = "geom_" + std::to_string(geometryRef);
                resolveTriangulatedFaceSet(entities, geometryRef, instance.vertices, instance.triangleIndices);
                break;
            }
        }

        if (!instance.sourceType.empty()) {
            result.instances.push_back(std::move(instance));
        }
    }

    return result;
}

std::string formatExtractionLogLine(const IfcInstance& instance) {
    std::ostringstream out;
    out << "instance=" << instance.instanceId
        << " geometry=" << instance.geometryId
        << " source=" << instance.sourceType;

    if (instance.sourceType == "IfcTriangulatedFaceSet") {
        out << " vertices=" << instance.vertices.size()
            << " triangleIndices=" << instance.triangleIndices.size();
    } else {
        size_t faceVertices = 0;
        for (const IfcFace& face : instance.faces) {
            faceVertices += face.vertices.size();
        }
        out << " faces=" << instance.faces.size()
            << " faceVertices=" << faceVertices;
    }

    return out.str();
}
