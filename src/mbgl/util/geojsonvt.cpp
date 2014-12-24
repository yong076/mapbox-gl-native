#include <mbgl/util/geojsonvt.hpp>
#include <mbgl/platform/log.hpp>

#include <queue>
#include <stack>
#include <cmath>

namespace mbgl {
namespace util {
namespace geojsonvt {

#pragma mark - Tile

Tile Tile::createTile(const std::vector<ProjectedFeature> &features, const uint8_t z2, const uint8_t tx, const uint8_t ty, const float tolerance, const float extent, const bool noSimplify) {

    Tile tile = Tile();

    for (uint32_t i = 0; i < features.size(); ++i) {
        tile.numFeatures++;
        addFeature(tile, features[i], z2, tx, ty, tolerance, extent, noSimplify);
    }

    return tile;
}

void Tile::addFeature(Tile &tile, const ProjectedFeature &feature, const uint8_t z2, const uint8_t tx, const uint8_t ty, const float tolerance, const float extent, const bool noSimplify) {

    ProjectedGeometryContainer *geom = (ProjectedGeometryContainer *)&feature.geometry;
    ProjectedFeatureType type = feature.type;
    std::vector<TileGeometry *> transformed;
    float sqTolerance = tolerance * tolerance;

    if (type == ProjectedFeatureType::Point) {
        for (uint32_t i = 0; i < geom->members.size(); ++i) {
            ProjectedPoint *p = (ProjectedPoint *)&geom->members[i];
            TilePoint tp = transformPoint(*p, z2, tx, ty, extent);
            transformed.push_back(&tp);
            tile.numPoints++;
            tile.numSimplified++;
        }
    } else {
        for (uint32_t i = 0; i < geom->members.size(); ++i) {
            ProjectedGeometryContainer *ring = (ProjectedGeometryContainer *)&geom->members[i];

            if (!noSimplify && ((type == ProjectedFeatureType::LineString && ring->dist < tolerance) ||
                (type == ProjectedFeatureType::Polygon && ring->area < sqTolerance))) {
                tile.numPoints += ring->members.size();
                continue;
            }

            TileRing transformedRing;

            for (uint32_t j = 0; j < ring->members.size(); ++j) {
                ProjectedPoint *p = (ProjectedPoint *)&ring->members[i];
                if (noSimplify || p->z > sqTolerance) {
                    TilePoint tp = transformPoint(*p, z2, tx, ty, extent);
                    transformedRing.points.push_back(tp);
                    tile.numSimplified++;
                }
                tile.numPoints++;
            }

            transformed.push_back(&transformedRing);
        }
    }

    if ((std::vector<TileGeometry> *)transformed.size() > 0) {
        tile.features.push_back(TileFeature(transformed, type, feature.tags));
    }

}

TilePoint Tile::transformPoint(const ProjectedPoint &p, const uint8_t z2, const uint8_t tx, const uint8_t ty, const float extent) {

    uint16_t x = extent * (p.x * z2 - tx);
    uint16_t y = extent * (p.y * z2 - ty);

    return TilePoint(x, y);
}

#pragma mark - GeoJSONVT

GeoJSONVT::GeoJSONVT(const std::string &data, uint8_t baseZoom_, uint8_t maxZoom_, uint32_t maxPoints_, float tolerance_, bool debug_)
    : baseZoom(baseZoom_),
      maxZoom(maxZoom_),
      maxPoints(maxPoints_),
      tolerance(tolerance_),
      debug(debug_) {

    if (this->debug) {
        // time 'preprocess data'
    }

    uint32_t z2 = 1 << this->baseZoom;

    JSDocument deserializedData;
    deserializedData.Parse<0>(data.c_str());

    if (deserializedData.HasParseError()) {
        Log::Warning(Event::General, "invalid GeoJSON");
        return;
    }

    std::vector<ProjectedFeature> features = Convert::convert(deserializedData, this->tolerance / (z2 * this->extent));

    if (this->debug) {
        // time end 'preprocess data'
        // time 'generate tiles up to z' + maxZoom
    }

    splitTile(features, 0, 0, 0);

    if (this->debug) {
        // log "features: %i, points: %i", self.tiles[0]!.numFeatures, self.tiles[0]!.numPoints)
        // time end 'generate tiles up to z\(maxZoom)")
        // log "tiles generated: %i %@", self.total, self.stats)
    }
}

void GeoJSONVT::splitTile(std::vector<ProjectedFeature> &features_, uint8_t z_, uint8_t x_, uint8_t y_, int8_t cz, int8_t cx, int8_t cy) {

    std::queue<FeatureStackItem> stack;
    stack.emplace(features_, z_, x_, y_);

    while (stack.size()) {
        FeatureStackItem set = stack.front();
        stack.pop();
        std::vector<ProjectedFeature> features = set.features;
        uint8_t z = set.z;
        uint8_t x = set.x;
        uint8_t y = set.y;

        uint32_t z2 = 1 << z;
        uint64_t id = toID(z, x, y);
        Tile tile;
        float tileTolerance = (z == this->baseZoom ? 0 : this->tolerance / (z2 * this->extent));

        if (this->tiles.count(id)) {
            tile = this->tiles[id];
        } else {
            if (this->debug) {
                // time 'creation'
            }

            tile = Tile::createTile(features, z2, x, y, tileTolerance, extent, (z == this->baseZoom));

            this->tiles[id] = tile;

            if (this->debug) {
//                NSLog("tile z%i-%i-%i (features: %i, points: %i, simplified: %i", z, x, y,
//                     tile.numFeatures, tile.numPoints, tile.numSimplified)
//                Util.timeEnd("creation")
//
//                let key = "z\(z):"
//                if (self.stats.count - 1 >= z) {
//                    var value = self.stats[key]! + 1
//                    self.stats[key] = value
//                } else {
//                    self.stats[key] = 1
//                }
//                self.total++
            }
        }

        if ((cz < 0 && (z == this->maxZoom || tile.numPoints <= this->maxPoints ||
            isClippedSquare(tile.features, this->extent, this->buffer))) || z == this->baseZoom || z == cz) {
            tile.source = features;
            continue;
        }

        if (cz >= 0) {
            tile.source = features;
        } else {
            tile.source = {};
        }

        if (this->debug) {
//                Util.time("clipping")
        }

        float k1 = 0.5 * this->buffer / this->extent;
        float k2 = 0.5 - k1;
        float k3 = 0.5 + k1;
        float k4 = 1 + k1;

        std::vector<ProjectedFeature> tl;
        std::vector<ProjectedFeature> bl;
        std::vector<ProjectedFeature> tr;
        std::vector<ProjectedFeature> br;
        std::vector<ProjectedFeature> left;
        std::vector<ProjectedFeature> right;
        uint32_t m = 0;
        bool goLeft = false;
        bool goTop = false;

        if (cz >= 0) {
            m = 1 << (cz - z);
            goLeft = cx / m - x < 0.5;
            goTop  = cy / m - y < 0.5;
        }

        if (cz < 0 || goLeft) {
            left = Clip::clip(features, z2, x - k1, x + k3, 0, intersectX);
        }

        if (cz < 0 || !goLeft) {
            right = Clip::clip(features, z2, x + k2, x + k4, 0, intersectX);
        }

        if (left.size()) {
            if (cz < 0 || goTop) {
                tl = Clip::clip(left, z2, y - k1, y + k3, 1, intersectY);
            }

            if (cz < 0 || !goTop) {
                bl = Clip::clip(left, z2, y + k2, y + k4, 1, intersectY);
            }
        }

        if (right.size()) {
            if (cz < 0 || goTop) {
                tr = Clip::clip(right, z2, y - k1, y + k3, 1, intersectY);
            }

            if (cz < 0 || !goTop) {
                br = Clip::clip(right, z2, y + k2, y + k4, 1, intersectY);
            }
        }

        if (this->debug) {
//                Util.timeEnd("clipping")
        }

        if (tl.size()) stack.emplace(tl, z + 1, x * 2,     y * 2);
        if (bl.size()) stack.emplace(bl, z + 1, x * 2,     y * 2 + 1);
        if (tr.size()) stack.emplace(tr, z + 1, x * 2 + 1, y * 2);
        if (br.size()) stack.emplace(br, z + 1, x * 2 + 1, y * 2 + 1);
    }
}

Tile GeoJSONVT::getTile(uint8_t z, uint8_t x, uint8_t y) {

    uint64_t id = toID(z, x, y);
    if (this->tiles.count(id)) {
        return this->tiles[id];
    }

    if (this->debug) {
//        NSLog("drilling down to z%i-%i-%i", z, x, y)
    }

    uint8_t z0 = z;
    uint8_t x0 = x;
    uint8_t y0 = y;
    Tile parent;

    while (!parent && z0) {
        z0--;
        x0 = x0 / 2;
        y0 = y0 / 2;
        uint64_t checkID = toID(z0, x0, y0);
        if (this->tiles.count(checkID)) {
            parent = this->tiles[checkID];
        }
    }

    if (this->debug) {
//        NSLog("found parent tile z%i-%i-%i", z0, x0, y0)
    }

    if (parent.source.size()) {
        if (isClippedSquare(parent.features, this->extent, this->buffer)) {
            return parent;
        }

        if (this->debug) {
//            Util.time("drilling down")
        }

        splitTile(parent.source, z0, x0, y0, z, x, y);

        if (this->debug) {
//            Util.timeEnd("drilling down")
        }
    }

    if (this->tiles.count(id)) {
        return this->tiles[id];
    }

    return Tile();
}

bool GeoJSONVT::isClippedSquare(const std::vector<TileFeature> &features, float extent_, float buffer_) const {

    if (features.size() != 1) {
        return false;
    }

    TileFeature feature = features.front();

    if (feature.type != TileFeatureType::Polygon || feature.geometry.size() > 1) {
        return false;
    }

    TileRing *ring = (TileRing *)&feature.geometry.front();

    for (uint32_t i = 0; i < ring->points.size(); ++i) {
        TilePoint p = ring->points[i];
        if ((p.x != -buffer_ && p.x != extent_ + buffer_) ||
            (p.y != -buffer_ && p.y != extent_ + buffer_)) {
            return false;
        }
    }

    return true;
}

uint64_t GeoJSONVT::toID(uint32_t z, uint32_t x, uint32_t y) {

    return (((1 << z) * y + x) * 32) + z;
}

ProjectedPoint GeoJSONVT::intersectX(ProjectedPoint a, ProjectedPoint b, float x) {

    float r1 = x;
    float r2 = (x - a.x) * (b.y - a.y) / (b.x - a.x) + a.y;
    float r3 = 1;

    return ProjectedPoint(r1, r2, r3);
}

ProjectedPoint GeoJSONVT::intersectY(ProjectedPoint a, ProjectedPoint b, float y) {

    float r1 = (y - a.y) * (b.x - a.x) / (b.y - a.y) + a.x;
    float r2 = y;
    float r3 = 1;

    return ProjectedPoint(r1, r2, r3);
}

#pragma mark - Convert

std::vector<ProjectedFeature> Convert::convert(const JSDocument &data, float tolerance) {

    std::vector<ProjectedFeature> features;
    const JSValue &rawType = data["type"];

    if (std::string(rawType.GetString()) == "FeatureCollection") {
        if (data.HasMember("features")) {
            const JSValue &rawFeatures = data["features"];
            if (rawFeatures.IsArray()) {
                for (rapidjson::SizeType i = 0; i < rawFeatures.Size(); ++i) {
                    convertFeature(features, rawFeatures[i], tolerance);
                }
            }
        }
    } else if (std::string(data["type"].GetString()) == "Feature") {
        convertFeature(features, data, tolerance);
    } else {

        /* In this case, we want to pass the entire JSON document as the
         * value for key 'geometry' in a new JSON object, like so: 
         *
         * convertFeature(features, ["geometry": data], tolerance); (pseudo-code)
         *
         * Currently this fails due to lack of a proper copy constructor. 
         * Maybe use move semantics? */

//        JSValue feature;
//        feature.SetObject();
//        feature.AddMember("geometry", data, data.GetAllocator());
//        convertFeature(features, feature, tolerance);

    }

    return features;
}

void Convert::convertFeature(std::vector<ProjectedFeature> &features, const JSValue &feature, float tolerance) {

    const JSValue &geom = feature["geometry"];
    const JSValue &rawType = geom["type"];
    std::string type { rawType.GetString(), rawType.GetStringLength() };
    Tags tags;

    if (feature.HasMember("properties") && feature["properties"].IsObject()) {
        const JSValue &properties = feature["properties"];
        rapidjson::Value::ConstMemberIterator itr = properties.MemberBegin();
        for (; itr != properties.MemberEnd(); ++itr) {
            std::string key { itr->name.GetString(), itr->name.GetStringLength() };
            std::string val { itr->value.GetString(), itr->value.GetStringLength() };
            tags[key] = val;
        }
    }

    if (type == "Point") {

        std::array<double, 2> coordinates = {{ 0, 0 }};
        if (geom.HasMember("coordinates")) {
            const JSValue &rawCoordinates = geom["coordinates"];
            if (rawCoordinates.IsArray()) {
                coordinates[0] = rawCoordinates[(rapidjson::SizeType)0].GetDouble();
                coordinates[1] = rawCoordinates[(rapidjson::SizeType)1].GetDouble();
            }
        }
        ProjectedPoint point = projectPoint(LonLat(coordinates));

        std::vector<ProjectedGeometry *> members = { &point };
        ProjectedGeometryContainer geometry(members);

        features.push_back(create(tags, ProjectedFeatureType::Point, geometry));

    } else if (type == "MultiPoint") {

        std::vector<std::array<double, 2>> coordinatePairs;
        std::vector<LonLat> points;
        if (geom.HasMember("coordinates")) {
            const JSValue &rawCoordinatePairs = geom["coordinates"];
            if (rawCoordinatePairs.IsArray()) {
                for (rapidjson::SizeType i = 0; i < rawCoordinatePairs.Size(); ++i) {
                    std::array<double, 2> coordinates = {{ 0, 0 }};
                    const JSValue &rawCoordinates = rawCoordinatePairs[i];
                    if (rawCoordinates.IsArray()) {
                        coordinates[0] = rawCoordinates[(rapidjson::SizeType)0].GetDouble();
                        coordinates[1] = rawCoordinates[(rapidjson::SizeType)1].GetDouble();
                    }
                    points.push_back(LonLat(coordinates));
                }
            }
        }

        ProjectedGeometryContainer geometry = project(points);

        features.push_back(create(tags, ProjectedFeatureType::Point, geometry));

    } else if (type == "LineString") {

        std::vector<std::array<double, 2>> coordinatePairs;
        std::vector<LonLat> points;
        if (geom.HasMember("coordinates")) {
            const JSValue &rawCoordinatePairs = geom["coordinates"];
            if (rawCoordinatePairs.IsArray()) {
                for (rapidjson::SizeType i = 0; i < rawCoordinatePairs.Size(); ++i) {
                    std::array<double, 2> coordinates = {{ 0, 0 }};
                    const JSValue &rawCoordinates = rawCoordinatePairs[i];
                    if (rawCoordinates.IsArray()) {
                        coordinates[0] = rawCoordinates[(rapidjson::SizeType)0].GetDouble();
                        coordinates[1] = rawCoordinates[(rapidjson::SizeType)1].GetDouble();
                    }
                    points.push_back(LonLat(coordinates));
                }
            }
        }

        ProjectedGeometryContainer geometry({ project(points, tolerance) });

        features.push_back(create(tags, ProjectedFeatureType::LineString, geometry));

    } else if (type == "MultiLineString" || type == "Polygon") {

        ProjectedGeometryContainer rings;
        if (geom.HasMember("coordinates")) {
            const JSValue &rawLines = geom["coordinates"];
            if (rawLines.IsArray()) {
                for (rapidjson::SizeType i = 0; i < rawLines.Size(); ++i) {
                    const JSValue &rawCoordinatePairs = rawLines[i];
                    if (rawCoordinatePairs.IsArray()) {
                        std::vector<LonLat> points;
                        for (rapidjson::SizeType j = 0; j < rawCoordinatePairs.Size(); ++j) {
                            std::array<double, 2> coordinates = {{ 0, 0 }};
                            const JSValue &rawCoordinates = rawCoordinatePairs[j];
                            if (rawCoordinates.IsArray()) {
                                coordinates[0] = rawCoordinates[(rapidjson::SizeType)0].GetDouble();
                                coordinates[1] = rawCoordinates[(rapidjson::SizeType)1].GetDouble();
                            }
                            points.push_back(LonLat(coordinates));
                        }
                        ProjectedGeometryContainer ring = project(points, tolerance);
                        rings.members.push_back(&ring);
                    }
                }
            }
        }

        ProjectedFeatureType projectedType = (type == "Polygon" ?
            ProjectedFeatureType::Polygon :
            ProjectedFeatureType::LineString);

        ProjectedGeometryContainer *geometry = &rings;

        features.push_back(create(tags, projectedType, *geometry));

    }

    else if (type == "MultiPolygon") {

        ProjectedGeometryContainer rings;
        if (geom.HasMember("coordinates")) {
            const JSValue &rawPolygons = geom["coordinates"];
            if (rawPolygons.IsArray()) {
                for (rapidjson::SizeType i = 0; i < rawPolygons.Size(); ++i) {
                    std::vector<LonLat> points;
                    const JSValue &rawLines = rawPolygons[i];
                    for (rapidjson::SizeType j = 0; j < rawLines.Size(); ++j) {
                        std::array<double, 2> coordinates = {{ 0, 0 }};
                        const JSValue &rawCoordinatePairs = rawLines[i];
                        if (rawCoordinatePairs.IsArray()) {
                            coordinates[0] = rawCoordinatePairs[(rapidjson::SizeType)0].GetDouble();
                            coordinates[1] = rawCoordinatePairs[(rapidjson::SizeType)1].GetDouble();
                        }
                        points.push_back(LonLat(coordinates));
                    }
                    ProjectedGeometryContainer ring = project(points, tolerance);
                    rings.members.push_back(&ring);
                }
            }
        }

        ProjectedGeometryContainer *geometry = &rings;

        features.push_back(create(tags, ProjectedFeatureType::Polygon, *geometry));

    } else if (type == "GeometryCollection") {

        if (geom.HasMember("geometries")) {
            const JSValue &rawGeometries = geom["geometries"];
            if (rawGeometries.IsArray()) {
                for (rapidjson::SizeType i = 0; i < rawGeometries.Size(); ++i) {
                    convertFeature(features, rawGeometries[i], tolerance);
                }
            }
        }

    } else {

        Log::Warning(Event::General, "unsupported GeoJSON type: " + std::string { geom["type"].GetString(), geom["type"].GetStringLength() });

    }
}

ProjectedFeature Convert::create(const Tags &tags, ProjectedFeatureType type, const ProjectedGeometry &geometry) {

    ProjectedFeature feature = ProjectedFeature(geometry, type, tags);
    calcBBox(feature);

    return feature;
}

ProjectedGeometryContainer Convert::project(const std::vector<LonLat> &lonlats, float tolerance) {

    ProjectedGeometryContainer projected;
    for (uint32_t i = 0; i < lonlats.size(); ++i) {
        ProjectedGeometry p = projectPoint(lonlats[i]);
        projected.members.push_back(&p);
    }
    if (tolerance) {
        Simplify::simplify(projected, tolerance);
        calcSize(projected);
    }

    return projected;
}

ProjectedPoint Convert::projectPoint(const LonLat &p_) {

    float sine = std::sin(p_.lat * M_PI / 180);
    float x = p_.lon / 360 + 0.5;
    float y = 0.5 - 0.25 * std::log((1 + sine) / (1 - sine)) / M_PI;

    ProjectedPoint p;
    p.x = x;
    p.y = y;
    p.z = 0;

    return p;
}

void Convert::calcSize(ProjectedGeometryContainer &geometryContainer) {

    float area = 0, dist = 0;
    ProjectedPoint *a, *b = nullptr;

    for (uint32_t i = 0; i < geometryContainer.members.size(); ++i) {
        a = (b != nullptr && b->isValid() ? b : (ProjectedPoint *)&geometryContainer.members[i]);
        b = (ProjectedPoint *)&geometryContainer.members[i + 1];

        area += a->x * b->y - b->x * a->y;
        dist += std::abs(b->x - a->x) + std::abs(b->y - a->y);
    }

    geometryContainer.area = std::abs(area / 2);
    geometryContainer.dist = dist;
}

void Convert::calcBBox(ProjectedFeature &feature) {

    ProjectedGeometryContainer *geometry = (ProjectedGeometryContainer *)&feature.geometry;
    ProjectedPoint minPoint = feature.minPoint;
    ProjectedPoint maxPoint = feature.maxPoint;

    if (feature.type == ProjectedFeatureType::Point) {
        calcRingBBox(minPoint, maxPoint, *geometry);
    } else {
        for (uint32_t i = 0; i < geometry->members.size(); ++i) {
            ProjectedGeometryContainer *featureGeometry = (ProjectedGeometryContainer *)&geometry->members[i];
            calcRingBBox(minPoint, maxPoint, *featureGeometry);
        }
    }
}

void Convert::calcRingBBox(ProjectedPoint &minPoint, ProjectedPoint &maxPoint, const ProjectedGeometryContainer &geometry) {

    for (uint32_t i = 0; i < geometry.members.size(); ++i) {
        ProjectedPoint *p = (ProjectedPoint *)&geometry.members[i];
        minPoint.x = std::min(p->x, minPoint.x);
        maxPoint.x = std::max(p->x, maxPoint.x);
        minPoint.y = std::min(p->y, minPoint.y);
        maxPoint.y = std::max(p->y, maxPoint.y);
    }
}

#pragma mark - Simplify

void Simplify::simplify(ProjectedGeometryContainer &points, float tolerance) {

    const float sqTolerance = tolerance * tolerance;
    const size_t len = points.members.size();
    size_t first = 0;
    size_t last = len - 1;
    std::stack<size_t> stack;
    float maxSqDist = 0;
    float sqDist = 0;
    size_t index = 0;

    ((ProjectedPoint *)&points.members[first])->z = 1;
    ((ProjectedPoint *)&points.members[last])->z  = 1;

    while (last) {

        maxSqDist = 0;

        for (size_t i = (first + 1); i < last; ++i) {
            sqDist = getSqSegDist(*(ProjectedPoint *)&points.members[i],
                *(ProjectedPoint *)&points.members[first],
                *(ProjectedPoint *)&points.members[last]);

            if (sqDist > maxSqDist) {
                index = i;
                maxSqDist = sqDist;
            }
        }

        if (maxSqDist > sqTolerance) {
            ((ProjectedPoint *)&points.members[index])->z = maxSqDist;
            stack.push(first);
            stack.push(index);
            stack.push(index);
            stack.push(last);
        }

        if (stack.size()) {
            last = stack.top();
            stack.pop();
        } else {
            last = 0;
        }

        if (stack.size()) {
            first = stack.top();
            stack.pop();
        } else {
            first = 0;
        }
    }
}

float Simplify::getSqSegDist(ProjectedPoint p, ProjectedPoint a, ProjectedPoint b) {

    float x = a.x;
    float y = a.y;
    float dx = b.x - a.x;
    float dy = b.y - a.y;

    if (dx || dy) {

        const float t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / (dx * dx + dy * dy);

        if (t > 1) {
            x = b.x;
            y = b.y;
        } else if (t) {
            x += dx * t;
            y += dy * t;
        }
    }

    dx = p.x - x;
    dy = p.y - y;

    return dx * dx + dy * dy;
}

#pragma mark - Clip

std::vector<ProjectedFeature> Clip::clip(const std::vector<ProjectedFeature> &features, uint32_t scale, float k1, float k2, uint8_t axis, ProjectedPoint (*intersect)(ProjectedPoint, ProjectedPoint, float)) {

    k1 /= scale;
    k2 /= scale;

    std::vector<ProjectedFeature> clipped;

    for (size_t i = 0; i < features.size(); ++i) {

        const ProjectedFeature feature = features[i];
        const ProjectedGeometry geometry = feature.geometry;
        const ProjectedFeatureType type = feature.type;
        float min;
        float max;

        if (feature.minPoint.isValid()) {
            min = (axis == 0 ? feature.minPoint.x : feature.minPoint.y);
            max = (axis == 0 ? feature.maxPoint.x : feature.maxPoint.y);

            if (min >= k1 && max <= k2) {
                clipped.push_back(feature);
                continue;
            } else if (min > k2 || max < k1) {
                continue;
            }
        }

        ProjectedGeometryContainer slices;

        if (type == ProjectedFeatureType::Point) {
            slices = clipPoints(*(ProjectedGeometryContainer *)&geometry, k1, k2, axis);
        } else {
            slices = clipGeometry(*(ProjectedGeometryContainer *)&geometry, k1, k2, axis, intersect, (type == ProjectedFeatureType::Polygon));
        }

        if (slices.members.size()) {
            clipped.push_back(ProjectedFeature(slices, type, features[i].tags));
        }
    }

    return clipped;
}

ProjectedGeometryContainer Clip::clipPoints(const ProjectedGeometryContainer &geometry, float k1, float k2, uint8_t axis) {

    ProjectedGeometryContainer slice;

    for (size_t i = 0; i < geometry.members.size(); ++i) {
        ProjectedPoint *a = (ProjectedPoint *)&geometry.members[i];
        float ak = (axis == 0 ? a->x: a->y);

        if (ak >= k1 && ak <= k2) {
            slice.members.push_back(a);
        }
    }

    return slice;
}

ProjectedGeometryContainer Clip::clipGeometry(const ProjectedGeometryContainer &geometry, float k1, float k2, uint8_t axis, ProjectedPoint (*intersect)(ProjectedPoint, ProjectedPoint, float), bool closed) {

    ProjectedGeometryContainer slices;

    for (size_t i = 0; i < geometry.members.size(); ++i) {

        float ak = 0;
        float bk = 0;
        ProjectedPoint *b = nullptr;
        const ProjectedGeometryContainer *points = (ProjectedGeometryContainer *)&geometry.members[i];
        const float area = points->area;
        const float dist = points->dist;
        const size_t len = points->members.size();
        ProjectedPoint *a = nullptr;

        ProjectedGeometryContainer slice;

        for (size_t j = 0; j < (len - 1); ++j) {
            a = (b != nullptr && b->isValid() ? b : (ProjectedPoint *)&points->members[j]);
            b = (ProjectedPoint *)&points->members[j + 1];
            ak = (bk ? bk : (axis == 0 ? a->x : a->y));
            bk = (axis == 0 ? b->x : b->y);

            if (ak < k1) {
                if (bk > k2) {
                    ProjectedPoint i1 = intersect(*a, *b, k1);
                    slice.members.push_back(&i1);
                    ProjectedPoint i2 = intersect(*a, *b, k2);
                    slice.members.push_back(&i2);
                    if (!closed) {
                        slice = newSlice(slices, slice, area, dist);
                    }
                } else if (bk >= k1) {
                    ProjectedPoint i1 = intersect(*a, *b, k1);
                    slice.members.push_back(&i1);
                }
            } else if (ak > k2) {
                if (bk < k1) {
                    ProjectedPoint i1 = intersect(*a, *b, k2);
                    slice.members.push_back(&i1);
                    ProjectedPoint i2 = intersect(*a, *b, k1);
                    slice.members.push_back(&i2);
                    if (!closed) {
                        slice = newSlice(slices, slice, area, dist);
                    }
                } else if (bk <= k2) {
                    ProjectedPoint i1 = intersect(*a, *b, k2);
                    slice.members.push_back(&i1);
                }
            } else {
                slice.members.push_back((ProjectedPoint *)&a);

                if (bk < k1) {
                    ProjectedPoint i1 = intersect(*a, *b, k1);
                    slice.members.push_back(&i1);
                    if (!closed) {
                        slice = newSlice(slices, slice, area, dist);
                    }
                } else if (bk > k2) {
                    ProjectedPoint i1 = intersect(*a, *b, k2);
                    slice.members.push_back(&i1);
                    if (!closed) {
                        slice = newSlice(slices, slice, area, dist);
                    }
                }
            }
        }

        a = (ProjectedPoint *)&points->members[len - 1];
        ak = (axis == 0 ? a->x : a->y);

        if (ak >= k1 && ak <= k2) {
            slice.members.push_back(a);
        }

        if (closed && slice.members.size()) {
            const ProjectedPoint *first = (ProjectedPoint *)&slice.members[0];
            const ProjectedPoint *last  = (ProjectedPoint *)&slice.members[slice.members.size() - 1];
            if (first != last) {
                ProjectedPoint p = ProjectedPoint(first->x, first->y, first->z);
                slice.members.push_back(&p);
            }
        }

        newSlice(slices, slices, area, dist);
    }

    return slices;
}

ProjectedGeometryContainer Clip::newSlice(ProjectedGeometryContainer &slices, ProjectedGeometryContainer &slice, float area, float dist) {

    if (slice.members.size()) {
        slice.area = area;
        slice.dist = dist;
        slices.members.push_back((ProjectedGeometry *)&slice);
    }

    return ProjectedGeometryContainer();
}

}
}
}
