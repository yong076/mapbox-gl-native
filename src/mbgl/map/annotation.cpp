#include <mbgl/map/annotation.hpp>

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

uint32_t AnnotationManager::addPointAnnotation(LatLng point, const std::string& symbol) {
    std::vector<LatLng> points({ point });
    std::vector<const std::string> symbols({ symbol });
    return addPointAnnotations(points, symbols)[0];
}

std::vector<uint32_t> AnnotationManager::addPointAnnotations(std::vector<LatLng> points, std::vector<const std::string>& symbols) {
    std::vector<uint32_t> result;
    result.reserve(points.size());
    for (uint32_t i = 0; i < points.size(); ++i) {
        uint32_t id = nextID();
        annotations[id] = std::move(util::make_unique<Annotation>(AnnotationType::Point, std::vector<AnnotationSegment>({{ points }})));

        result.push_back(id);

        printf("%s\n", symbols[i].c_str());
    }

    // map.update(); ?

    return result;
}

uint32_t AnnotationManager::addShapeAnnotation(std::vector<AnnotationSegment> shape) {
    return addShapeAnnotations({ shape })[0];
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

void AnnotationManager::removeAnnotation(uint32_t id) {
    removeAnnotations({ id });
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

BoundingBox AnnotationManager::getBoundingBoxForAnnotations(const std::vector<uint32_t> ids) const {
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
