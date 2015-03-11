#ifndef MBGL_MAP_LIVE_TILE
#define MBGL_MAP_LIVE_TILE

#include <mbgl/map/geometry_tile.hpp>
#include <mbgl/util/geojsonvt.hpp>

namespace mbgl {

class LiveTile : public GeometryTile {
public:
//    LiveTile();
    LiveTile(mapbox::util::geojsonvt::Tile*);

    util::ptr<const GeometryTileLayer> getLayer(const std::string&) const override;

private:
    mapbox::util::geojsonvt::Tile* tile;
};

}

#endif
