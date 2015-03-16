#include <mbgl/map/live_tile.hpp>
#include <mbgl/util/constants.hpp>

namespace mbgl {

LiveTileFeature::LiveTileFeature(FeatureType type_, GeometryCollection geometries_, std::map<std::string, std::string> properties_)
    : type(type_),
      properties(properties_),
      geometries(geometries_) {}

LiveTileFeature::LiveTileFeature(TFeature& tile_feature) {
    type = (FeatureType)tile_feature.type;
    properties = tile_feature.tags;

    for (auto& tile_geometry : tile_feature.geometry) {
        if (tile_geometry.is<TPoint>()) {
            TPoint point = tile_geometry.get<TPoint>();
            geometries.push_back({ point });
        } else if (tile_geometry.is<TRing>()) {
            TRing ring = tile_geometry.get<TRing>();
            std::vector<Coordinate> line;
            for (auto& point : ring.points) {
                line.emplace_back(point.x, point.y);
            }
            geometries.push_back(line);
        }
    }
}

mapbox::util::optional<Value> LiveTileFeature::getValue(const std::string& key) const {
    auto it = properties.find(key);
    if (it != properties.end()) {
        return mapbox::util::optional<Value>(it->second);
    }
    return mapbox::util::optional<Value>();
}

LiveTileLayer::LiveTileLayer() {}

LiveTileLayer::LiveTileLayer(std::vector<TFeature>& tile_features) {
    for (auto& tile_feature : tile_features) {
        features.push_back(std::make_shared<LiveTileFeature>(tile_feature));
    }
}

void LiveTileLayer::prepareToAddFeatures(size_t count) {
    features.reserve(features.size() + count);
}

void LiveTileLayer::addFeature(util::ptr<const LiveTileFeature> feature) {
    features.push_back(std::move(feature));
}

LiveTile::LiveTile() {}

LiveTile::LiveTile(TTile* tile_)
    : tile(tile_) {
    convert();
}

void LiveTile::addLayer(const std::string& name, util::ptr<LiveTileLayer> layer) {
    layers.emplace(name, std::move(layer));
}

void LiveTile::convert() {
    if (tile == nullptr || !tile) return;

    layers.emplace(util::ANNOTATIONS_POINTS_LAYER_ID, std::make_shared<LiveTileLayer>(tile->features));
}

util::ptr<const GeometryTileLayer> LiveTile::getLayer(const std::string& name) const {
    auto layer_it = layers.find(name);
    if (layer_it != layers.end()) {
        return layer_it->second;
    }
    return nullptr;
}

}
