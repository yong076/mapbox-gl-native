#include <mbgl/map/annotation.hpp>
#include <mbgl/util/ptr.hpp>

#include <memory>

using namespace mbgl;

Annotation::Annotation(AnnotationType type_, std::vector<AnnotationSegment> geometry_)
    : type(type_),
      geometry(geometry_) {
    if (type == AnnotationType::Point) {
        bbox = BoundingBox(getPoint(), getPoint());
    } else {
        LatLng sw, ne;
        for (auto segment : geometry) {
            for (auto point : segment) {
                if (point.latitude < sw.latitude) sw.latitude = point.latitude;
                if (point.latitude > ne.latitude) ne.latitude = point.latitude;
                if (point.longitude < sw.longitude) sw.longitude = point.longitude;
                if (point.longitude > ne.longitude) ne.longitude = point.longitude;
            }
        }
        bbox = BoundingBox(sw, ne);
    }
}

AnnotationManager::AnnotationManager(Map& map_)
    : map(map_),
    nullTile(util::make_unique<LiveTile>()) {}

vec2<double> AnnotationManager::projectPoint(LatLng& point) {
    double sine = std::sin(point.latitude * M_PI / 180);
    double x = point.longitude / 360 + 0.5;
    double y = 0.5 - 0.25 * std::log((1 + sine) / (1 - sine)) / M_PI;
    return vec2<double>(x, y);
}

std::pair<std::vector<Tile::ID>, std::vector<uint32_t>> AnnotationManager::addPointAnnotations(std::vector<LatLng> points, std::vector<std::string>& symbols) {

    uint16_t extent = 4096;

    std::vector<uint32_t> result;
    result.reserve(points.size());

    std::vector<Tile::ID> affectedTiles;

    for (uint32_t i = 0; i < points.size(); ++i) {
        uint32_t annotationID = nextID();

        uint8_t maxZoom = map.getMaxZoom();

        uint32_t z2 = 1 << maxZoom;

        vec2<double> p = projectPoint(points[i]);

        uint32_t x = p.x * z2;
        uint32_t y = p.y * z2;

        for (int8_t z = maxZoom; z >= 0; z--) {
            affectedTiles.emplace_back(z, x, y);
            Tile::ID tileID = affectedTiles.back();
            Coordinate coordinate(extent * (p.x * z2 - x), extent * (p.y * z2 - y));

            GeometryCollection geometries({{ {{ coordinate }} }});

            std::map<std::string, std::string> properties = {{ "sprite", (symbols[i].length() ? symbols[i] : defaultPointAnnotationSymbol) }};

            auto feature = std::make_shared<const LiveTileFeature>(FeatureType::Point,
                                                                   geometries,
                                                                   properties);

            auto tile_it = annotationTiles.find(tileID);
            if (tile_it != annotationTiles.end()) {
                auto layer = tile_it->second->getLayer(util::ANNOTATIONS_POINTS_LAYER_ID);
                auto mutableLayer = std::const_pointer_cast<GeometryTileLayer>(layer);
                auto liveLayer = std::static_pointer_cast<LiveTileLayer>(mutableLayer);
                liveLayer->addFeature(feature);
            } else {
                util::ptr<LiveTileLayer> layer = std::make_shared<LiveTileLayer>();
                layer->addFeature(feature);
                auto tile_pos = annotationTiles.emplace(tileID, util::make_unique<LiveTile>());
                tile_pos.first->second->addLayer(util::ANNOTATIONS_POINTS_LAYER_ID, layer);
            }

            z2 /= 2;
            x /= 2;
            y /= 2;
        }

        annotations.emplace(annotationID, util::make_unique<Annotation>(AnnotationType::Point, std::vector<AnnotationSegment>({{ points[i] }})));

        result.push_back(annotationID);
    }

    return std::make_pair(affectedTiles, result);
}

std::vector<uint32_t> AnnotationManager::addShapeAnnotations(std::vector<std::vector<AnnotationSegment>> shapes) {
    std::vector<uint32_t> result;
    result.reserve(shapes.size());
    for (uint32_t i = 0; i < shapes.size(); ++i) {
        uint32_t id = nextID();
        annotations[id] = std::move(util::make_unique<Annotation>(AnnotationType::Shape, shapes[i]));

        result.push_back(id);
    }

    // map.update(); ?

    return result;
}

void AnnotationManager::removeAnnotations(std::vector<uint32_t> ids) {
    for (auto id : ids) {
        annotations.erase(id);
    }

    // map.update(); ?
}

std::vector<uint32_t> AnnotationManager::getAnnotationsInBoundingBox(BoundingBox bbox) const {
    printf("%f, %f\n", bbox.sw.latitude, bbox.sw.longitude);
    return {};
}

BoundingBox AnnotationManager::getBoundingBoxForAnnotations(std::vector<uint32_t> ids) const {
    LatLng sw, ne;
    for (auto id : ids) {
        auto annotation_it = annotations.find(id);
        if (annotation_it != annotations.end()) {
            if (annotation_it->second->bbox.sw.latitude < sw.latitude) sw.latitude = annotation_it->second->bbox.sw.latitude;
            if (annotation_it->second->bbox.ne.latitude > ne.latitude) ne.latitude = annotation_it->second->bbox.ne.latitude;
            if (annotation_it->second->bbox.sw.longitude < sw.longitude) sw.longitude = annotation_it->second->bbox.sw.longitude;
            if (annotation_it->second->bbox.ne.longitude > ne.longitude) ne.longitude = annotation_it->second->bbox.ne.longitude;
        }
    }

    return BoundingBox(sw, ne);
}

const std::unique_ptr<LiveTile>& AnnotationManager::getTile(Tile::ID const& id) {
    std::lock_guard<std::mutex> lock(mtx);

    auto tile_it = annotationTiles.find(id);
    if (tile_it != annotationTiles.end()) {
        return tile_it->second;
    }
    return nullTile;
}
