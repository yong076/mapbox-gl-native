#ifndef MBGL_UTIL_GEOJSONVT
#define MBGL_UTIL_GEOJSONVT

#include <array>
#include <vector>
#include <map>
#include <string>

#include <mbgl/util/time.hpp>
#include <mbgl/util/variant.hpp>

#include <rapidjson/document.h>

namespace mbgl {
namespace util {
namespace geojsonvt {

class Time {
public:
    inline static void time(std::string activity) {
        Time::activities[activity] = now();
    }
    inline static void timeEnd(std::string activity) {
        printf("%s: %fms\n", activity.c_str(), (now() - Time::activities[activity]) / 1e6);
    }

private:
    static std::map<std::string, timestamp> activities;
};

struct LonLat {
    LonLat(std::array<double, 2> coordinates)
        : lon(coordinates[0]), lat(coordinates[1]) {}

    double lon;
    double lat;
};

class ProjectedPoint;
class ProjectedGeometryContainer;

using ProjectedGeometry = mapbox::util::variant<geojsonvt::ProjectedPoint, geojsonvt::ProjectedGeometryContainer>;

class ProjectedPoint {
public:
    ProjectedPoint(float x_, float y_, float z_)
        : x(x_), y(y_), z(z_) {}
    ProjectedPoint()
        : x(-1), y(-1), z(-1) {}

    inline bool isValid() const { return (x >= 0 && y >= 0 && z >= 0); }
    inline bool operator==(const ProjectedPoint rhs) const {
        return (x == rhs.x && y == rhs.y && z == rhs.z);
    }

public:
    float x = -1;
    float y = -1;
    float z = -1;
};

using JSDocument = rapidjson::Document;
using JSValue = rapidjson::Value;

class ProjectedGeometryContainer {
public:
    ProjectedGeometryContainer() {}
    ProjectedGeometryContainer(std::vector<ProjectedGeometry> members_)
        : members(members_) {}

public:
    std::vector<ProjectedGeometry> members;
    double area = 0;
    double dist = 0;
};

using Tags = std::map<std::string, std::string>;

enum class ProjectedFeatureType: uint8_t {
    Point,
    LineString,
    Polygon
};

class ProjectedFeature {
public:
    ProjectedFeature(ProjectedGeometry geometry_, ProjectedFeatureType type_, Tags tags_)
        : geometry(geometry_), type(type_), tags(tags_) {}

public:
    ProjectedGeometry geometry;
    ProjectedFeatureType type;
    Tags tags;
    ProjectedPoint minPoint = ProjectedPoint(1, 1, 0);
    ProjectedPoint maxPoint = ProjectedPoint(0, 0, 0);
};

class TilePoint;
class TileRing;

using TileGeometry = mapbox::util::variant<geojsonvt::TilePoint, geojsonvt::TileRing>;

class TilePoint {
public:
    TilePoint(uint16_t x_, uint16_t y_)
        : x(x_), y(y_) {}

public:
    const uint16_t x = 0;
    const uint16_t y = 0;
};

class TileRing {
public:
    std::vector<TilePoint> points;
};

typedef ProjectedFeatureType TileFeatureType;

class TileFeature {
public:
    TileFeature(std::vector<TileGeometry> geometry_, TileFeatureType type_, Tags tags_)
        : geometry(geometry_), type(type_), tags(tags_) {}

public:
    std::vector<TileGeometry> geometry;
    TileFeatureType type;
    Tags tags;
};

class Tile {
public:
    static Tile createTile(std::vector<ProjectedFeature> features, uint8_t z2, uint8_t tx, uint8_t ty, double tolerance, float extent, bool noSimplify);

    static void addFeature(Tile &tile, ProjectedFeature feature, uint8_t z2, uint8_t tx, uint8_t ty, double tolerance, float extent, bool noSimplify);

    inline operator bool() const { return this->numPoints > 0; }

private:
    static TilePoint transformPoint(const ProjectedPoint &p, uint8_t z2, uint8_t tx, uint8_t ty, float extent);

public:
    std::vector<TileFeature> features;
    uint32_t numPoints = 0;
    uint32_t numSimplified = 0;
    uint32_t numFeatures = 0;
    std::vector<ProjectedFeature> source;
};

class GeoJSONVT {
public:
    GeoJSONVT(const std::string &data, uint8_t baseZoom = 14, uint8_t maxZoom = 4, uint32_t maxPoints = 100, double tolerance = 3, bool debug = true);

    Tile& getTile(uint8_t z, uint8_t x, uint8_t y);

private:
    void splitTile(std::vector<ProjectedFeature> features, uint8_t z, uint8_t x, uint8_t y, int8_t cz = -1, int8_t cx = -1, int8_t cy = -1);

    bool isClippedSquare(const std::vector<TileFeature> features, float extent, float buffer) const;

    static uint64_t toID(uint32_t z, uint32_t x, uint32_t y);

    static ProjectedPoint intersectX(const ProjectedPoint &a, const ProjectedPoint &b, float x);

    static ProjectedPoint intersectY(const ProjectedPoint &a, const ProjectedPoint &b, float y);

    struct FeatureStackItem {
        std::vector<ProjectedFeature> &features;
        uint8_t z;
        uint8_t x;
        uint8_t y;

        FeatureStackItem(std::vector<ProjectedFeature> &features_, uint8_t z_, uint8_t x_, uint8_t y_)
            : features(features_), z(z_), x(x_), y(y_) {}
    };

private:
    uint8_t baseZoom;
    uint8_t maxZoom;
    uint32_t maxPoints;
    double tolerance;
    bool debug;
    uint16_t extent = 4096;
    uint8_t buffer = 64;
    std::map<uint64_t, Tile> tiles;
    std::map<std::string, uint8_t> stats;
    uint16_t total = 0;
};

class Convert {
public:
    static std::vector<ProjectedFeature> convert(const JSDocument &data, double tolerance);

private:
    static void convertFeature(std::vector<ProjectedFeature> &features, const JSValue &feature, double tolerance);

    static ProjectedFeature create(Tags tags, ProjectedFeatureType type, ProjectedGeometry geometry);

    static ProjectedGeometryContainer project(const std::vector<LonLat> &lonlats, double tolerance = 0);

    static ProjectedPoint projectPoint(const LonLat &p);

    static void calcSize(ProjectedGeometryContainer &geometryContainer);

    static void calcBBox(ProjectedFeature &feature);

    static void calcRingBBox(ProjectedPoint &minPoint, ProjectedPoint &maxPoint, const ProjectedGeometryContainer &geometry);
};

class Simplify {
public:
    static void simplify(ProjectedGeometryContainer &points, double tolerance);

private:
    static float getSqSegDist(const ProjectedPoint &p, const ProjectedPoint &a, const ProjectedPoint &b);
};

class Clip {
public:
    static std::vector<ProjectedFeature> clip(std::vector<ProjectedFeature> features, uint32_t scale, float k1, float k2, uint8_t axis, ProjectedPoint (*intersect)(const ProjectedPoint&, const ProjectedPoint&, float));

private:
    static ProjectedGeometryContainer clipPoints(ProjectedGeometryContainer geometry, float k1, float k2, uint8_t axis);

    static ProjectedGeometryContainer clipGeometry(ProjectedGeometryContainer geometry, float k1, float k2, uint8_t axis, ProjectedPoint (*intersect)(const ProjectedPoint&, const ProjectedPoint&, float), bool closed);

    static ProjectedGeometryContainer newSlice(ProjectedGeometryContainer &slices, ProjectedGeometryContainer &slice, double area, double dist);
};

} // namespace geojsonvt
} // namespace util
} // namespace mbgl

#endif
