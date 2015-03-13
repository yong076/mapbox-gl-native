#include <mbgl/map/annotation.hpp>

using namespace mbgl;

void AnnotationManager::setDefaultPointAnnotationSymbol(const std::string& symbol) {
    printf("%s\n", symbol.c_str());
}

uint64_t AnnotationManager::addPointAnnotation(const LatLng point, const std::string& symbol) {
    printf("%f, %f %s\n", point.latitude, point.longitude, symbol.c_str());
    return 0;
}

std::vector<const uint64_t> AnnotationManager::addPointAnnotations(const std::vector<LatLng> points, const std::vector<const std::string>& symbols) {
    printf("%f, %f %s\n", points[0].latitude, points[0].longitude, symbols[0].c_str());
    return {};
}

uint64_t AnnotationManager::addShapeAnnotation(const std::vector<AnnotationSegment> shape) {
    printf("%f, %f\n", shape[0][0].latitude, shape[0][0].longitude);
    return 0;
}

std::vector<const uint64_t> AnnotationManager::addShapeAnnotations(const std::vector<const std::vector<AnnotationSegment>> shapes) {
    printf("%f, %f\n", shapes[0][0][0].latitude, shapes[0][0][0].longitude);
    return {};
}

void AnnotationManager::removeAnnotation(const uint64_t annotation) {
    printf("%llu\n", annotation);
}

void AnnotationManager::removeAnnotations(const std::vector<const uint64_t> annotations_) {
    printf("%llu\n", annotations_[0]);
}

std::vector<const uint64_t> AnnotationManager::getAnnotationsInBoundingBox(BoundingBox bbox) const {
    printf("%f, %f\n", bbox.sw.latitude, bbox.sw.longitude);
    return {};
}

BoundingBox AnnotationManager::getBoundingBoxForAnnotations(const std::vector<const uint64_t> annotations_) const {
    printf("%llu\n", annotations_[0]);
    return BoundingBox();
}
