#include <mbgl/map/live_tile.hpp>

namespace mbgl {

LiveTile::LiveTile(mapbox::util::geojsonvt::Tile* tile_)
    : tile(tile_) {}

util::ptr<const GeometryTileLayer> LiveTile::getLayer(const std::string& name) const {
//    auto layer_it = layers.find(name);
//    if (layer_it != layers.end()) {
//        return layer_it->second;
//    }
    if (name.length()) {}
    if (tile) {}

    return nullptr;
}

}
