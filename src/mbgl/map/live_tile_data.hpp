#ifndef MBGL_MAP_LIVE_TILE_DATA
#define MBGL_MAP_LIVE_TILE_DATA

#include <mbgl/map/vector_tile_data.hpp>
#include <mbgl/util/geojsonvt.hpp>

namespace mbgl {

class LiveTileData : public VectorTileData {
public:
    LiveTileData(Tile::ID const&,
                 float mapMaxZoom,
                 util::ptr<Style>,
                 GlyphAtlas&,
                 GlyphStore&,
                 SpriteAtlas&,
                 util::ptr<Sprite>,
                 const SourceInfo&,
                 Environment&,
                 util::ptr<mapbox::util::geojsonvt::GeoJSONVT>&);

    void parse() override;

protected:
    util::ptr<mapbox::util::geojsonvt::GeoJSONVT>& geojsonvt;
};

}

#endif
