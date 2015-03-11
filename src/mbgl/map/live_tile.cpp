#include <mbgl/map/live_tile.hpp>

namespace mbgl {

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

LiveTileLayer::LiveTileLayer(std::vector<TFeature>& tile_features) {
    for (auto& tile_feature : tile_features) {
        features.push_back(std::make_shared<LiveTileFeature>(tile_feature));
    }
}

LiveTile::LiveTile(TTile* tile_)
    : tile(tile_) {
    convert();
}

void LiveTile::convert() {
    if (tile == nullptr || !tile) return;

    layers.emplace("annotations", std::make_shared<LiveTileLayer>(tile->features));
}

util::ptr<const GeometryTileLayer> LiveTile::getLayer(const std::string& name) const {
    auto layer_it = layers.find(name);
    if (layer_it != layers.end()) {
        return layer_it->second;
    }
    return nullptr;
}

}
