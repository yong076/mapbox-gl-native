#ifndef MBGL_MAP_ANNOTATIONS
#define MBGL_MAP_ANNOTATIONS

#include <mbgl/map/map.hpp>
#include <mbgl/map/tile.hpp>
#include <mbgl/map/live_tile.hpp>
#include <mbgl/util/geo.hpp>
#include <mbgl/util/noncopyable.hpp>

#include <string>
#include <vector>
#include <map>

namespace mbgl {

class Annotation;

enum class AnnotationType : uint8_t {
    Point,
    Shape
};

class AnnotationManager : private util::noncopyable {
public:
    void setDefaultPointAnnotationSymbol(const std::string&);
    uint64_t addPointAnnotation(const LatLng, const std::string& symbol = "");
    std::vector<const uint64_t> addPointAnnotations(const std::vector<LatLng>, const std::vector<const std::string>& symbols = {{}});
    uint64_t addShapeAnnotation(const std::vector<AnnotationSegment>);
    std::vector<const uint64_t> addShapeAnnotations(const std::vector<const std::vector<AnnotationSegment>>);
    void removeAnnotation(const uint64_t);
    void removeAnnotations(const std::vector<const uint64_t>);
    std::vector<const uint64_t> getAnnotationsInBoundingBox(BoundingBox) const;
    BoundingBox getBoundingBoxForAnnotations(const std::vector<const uint64_t>) const;

private:
    std::map<std::string, Annotation> annotations;
    std::map<Tile::ID, LiveTile> annotationTiles;
};

class Annotation : private util::noncopyable {
public:
    LatLng getPoint() const { return geometry[0][0]; }

public:
    const std::string id;
    const AnnotationType type = AnnotationType::Point;
    const std::vector<AnnotationSegment> geometry;
    const BoundingBox bbox;
    std::map<Tile::ID, std::vector<LiveTileFeature>> tileFeatures;
};

}

#endif
