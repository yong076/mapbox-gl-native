#ifndef MBGL_MAP_ANNOTATIONS
#define MBGL_MAP_ANNOTATIONS

#include <mbgl/map/map.hpp>
#include <mbgl/map/tile.hpp>
#include <mbgl/map/live_tile.hpp>
#include <mbgl/util/geo.hpp>
#include <mbgl/util/noncopyable.hpp>
#include <mbgl/util/std.hpp>
#include <mbgl/util/vec.hpp>

#include <string>
#include <vector>
#include <map>
#include <mutex>

namespace mbgl {

class Annotation;

enum class AnnotationType : uint8_t {
    Point,
    Shape
};

class AnnotationManager : private util::noncopyable {
public:
    AnnotationManager(Map&);

    void setDefaultPointAnnotationSymbol(std::string& symbol) { defaultPointAnnotationSymbol = symbol; }
    std::pair<std::vector<Tile::ID>, std::vector<uint32_t>> addPointAnnotations(std::vector<LatLng>, std::vector<std::string>& symbols);
    std::vector<uint32_t> addShapeAnnotations(std::vector<std::vector<AnnotationSegment>>);
    void removeAnnotations(std::vector<uint32_t>);
    std::vector<uint32_t> getAnnotationsInBoundingBox(BoundingBox) const;
    BoundingBox getBoundingBoxForAnnotations(std::vector<uint32_t>) const;

    const std::unique_ptr<LiveTile>& getTile(Tile::ID const& id);

private:
    uint32_t nextID() { return nextID_++; }
    static vec2<double> projectPoint(LatLng& point);

private:
    std::mutex mtx;
    Map& map;
    std::string defaultPointAnnotationSymbol = "marker-red";
    std::map<uint32_t, std::unique_ptr<Annotation>> annotations;
    std::map<Tile::ID, std::unique_ptr<LiveTile>> annotationTiles;
    std::unique_ptr<LiveTile> nullTile;
    uint32_t nextID_ = 0;
};

class Annotation : private util::noncopyable {
public:
    Annotation(AnnotationType, std::vector<AnnotationSegment>);

    std::vector<AnnotationSegment> getGeometry() const { return geometry; }
    LatLng getPoint() const { return geometry[0][0]; }
    BoundingBox getBoundingBox() const { return bbox; }

public:
    const AnnotationType type = AnnotationType::Point;
    const std::vector<AnnotationSegment> geometry;
    BoundingBox bbox;
    std::map<Tile::ID, std::vector<LiveTileFeature>> tileFeatures;
};

}

#endif
