#ifndef MBGL_UTIL_GEOJSONVT
#define MBGL_UTIL_GEOJSONVT

#include <array>
#include <vector>
#include <map>
#include <string>

#include <rapidjson/document.h>

namespace mbgl {
namespace util {
namespace geojsonvt {

struct LonLat {
    LonLat(std::array<double, 2> coordinates)
        : lon(coordinates[0]), lat(coordinates[1]) {}

    double lon;
    double lat;
};

class ProjectedGeometry {
    // abstract
};

class ProjectedPoint: public ProjectedGeometry {
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

class ProjectedGeometryContainer: public ProjectedGeometry {
public:
    ProjectedGeometryContainer() {}
    ProjectedGeometryContainer(const std::vector<ProjectedGeometry *> &members_)
        : members(members_) {}

public:
    std::vector<ProjectedGeometry *> members;
    float area = 0;
    float dist = 0;
};

using Tags = std::map<std::string, std::string>;

enum class ProjectedFeatureType: uint8_t {
    Point,
    LineString,
    Polygon
};

class ProjectedFeature {
public:
    ProjectedFeature(const ProjectedGeometry &geometry_, ProjectedFeatureType type_, const Tags &tags_)
        : geometry(geometry_), type(type_), tags(tags_) {}

public:
    ProjectedGeometry geometry;
    ProjectedFeatureType type;
    Tags tags;
    ProjectedPoint minPoint = ProjectedPoint();
    ProjectedPoint maxPoint = ProjectedPoint();
};

class TileGeometry {
    // abstract
};

class TilePoint: public TileGeometry {
public:
    TilePoint(uint16_t x_, uint16_t y_)
        : x(x_), y(y_) {}

public:
    const uint16_t x = 0;
    const uint16_t y = 0;
};

class TileRing: public TileGeometry {
public:
    std::vector<TilePoint> points;
};

typedef ProjectedFeatureType TileFeatureType;

class TileFeature {
public:
    TileFeature(const std::vector<TileGeometry *> geometry_, TileFeatureType type_, const Tags &tags_)
        : geometry(geometry_), type(type_), tags(tags_) {}

public:
    std::vector<TileGeometry *> geometry;
    TileFeatureType type;
    Tags tags;
};

class Tile {
public:
    static Tile createTile(const std::vector<ProjectedFeature> &features, uint8_t z2, uint8_t tx, uint8_t ty, float tolerance, float extent, bool noSimplify);

    static void addFeature(Tile &tile, const ProjectedFeature &feature, uint8_t z2, uint8_t tx, uint8_t ty, float tolerance, float extent, bool noSimplify);

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
    GeoJSONVT(const std::string &data, uint8_t baseZoom = 14, uint8_t maxZoom = 4, uint32_t maxPoints = 100, float tolerance = 3, bool debug = false);

    Tile getTile(uint8_t z, uint8_t x, uint8_t y);

private:
    void splitTile(std::vector<ProjectedFeature> &features, uint8_t z, uint8_t x, uint8_t y, int8_t cz = -1, int8_t cx = -1, int8_t cy = -1);

    bool isClippedSquare(const std::vector<TileFeature> &features, float extent, float buffer) const;

    static uint64_t toID(uint32_t z, uint32_t x, uint32_t y);

    static ProjectedPoint intersectX(ProjectedPoint a, ProjectedPoint b, float x);

    static ProjectedPoint intersectY(ProjectedPoint a, ProjectedPoint b, float y);

    struct FeatureStackItem {
        std::vector<ProjectedFeature> features;
        uint8_t z;
        uint8_t x;
        uint8_t y;

        FeatureStackItem(const std::vector<ProjectedFeature> &features_, uint8_t z_, uint8_t x_, uint8_t y_)
            : features(features_), z(z_), x(x_), y(y_) {}
    };

private:
    uint8_t baseZoom;
    uint8_t maxZoom;
    uint32_t maxPoints;
    float tolerance;
    bool debug;
    uint16_t extent = 4096;
    uint8_t buffer = 64;
    std::map<uint64_t, Tile> tiles;
    std::map<std::string, uint8_t> stats;
//    uint16_t total = 0;
};

class Convert {
public:
    static std::vector<ProjectedFeature> convert(const JSDocument &data, float tolerance);

private:
    static void convertFeature(std::vector<ProjectedFeature> &features, const JSValue &feature, float tolerance);

    static ProjectedFeature create(const Tags &tags, ProjectedFeatureType type, const ProjectedGeometry &geometry);

    static ProjectedGeometryContainer project(const std::vector<LonLat> &lonlats, float tolerance = 0);

    static ProjectedPoint projectPoint(const LonLat &p);

    static void calcSize(ProjectedGeometryContainer &geometryContainer);

    static void calcBBox(ProjectedFeature &feature);

    static void calcRingBBox(ProjectedPoint &minPoint, ProjectedPoint &maxPoint, const ProjectedGeometryContainer &geometry);
};

class Simplify {
public:
    static void simplify(ProjectedGeometryContainer &points, float tolerance);

private:
    static float getSqSegDist(ProjectedPoint p, ProjectedPoint a, ProjectedPoint b);
};

class Clip {
public:
    static std::vector<ProjectedFeature> clip(const std::vector<ProjectedFeature> &features, uint32_t scale, float k1, float k2, uint8_t axis, ProjectedPoint (*intersect)(ProjectedPoint, ProjectedPoint, float));

private:
    static ProjectedGeometryContainer clipPoints(const ProjectedGeometryContainer &geometry, float k1, float k2, uint8_t axis);

    static ProjectedGeometryContainer clipGeometry(const ProjectedGeometryContainer &geometry, float k1, float k2, uint8_t axis, ProjectedPoint (*intersect)(ProjectedPoint, ProjectedPoint, float), bool closed);

    static ProjectedGeometryContainer newSlice(ProjectedGeometryContainer &slices, ProjectedGeometryContainer &slice, float area, float dist);
};

} // namespace geojsonvt
} // namespace util
} // namespace mbgl

#endif
