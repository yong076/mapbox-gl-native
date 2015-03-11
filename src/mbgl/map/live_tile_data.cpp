#include <mbgl/map/live_tile_data.hpp>
#include <mbgl/map/live_tile_parser.hpp>
#include <mbgl/map/vector_tile.hpp>
#include <mbgl/platform/log.hpp>

using namespace mbgl;

LiveTileData::LiveTileData(Tile::ID const& id_,
                           float mapMaxZoom,
                           util::ptr<Style> style_,
                           GlyphAtlas& glyphAtlas_,
                           GlyphStore& glyphStore_,
                           SpriteAtlas& spriteAtlas_,
                           util::ptr<Sprite> sprite_,
                           const SourceInfo& source_,
                           Environment& env_,
                           util::ptr<mapbox::util::geojsonvt::GeoJSONVT>& geojsonvt_)
    : VectorTileData::VectorTileData(id_, mapMaxZoom, style_, glyphAtlas_, glyphStore_,
                                     spriteAtlas_, sprite_, source_, env_),
      geojsonvt(geojsonvt_) {
    // GeoJSON is already loaded by now
    state = State::loaded;
}

LiveTileData::~LiveTileData() {
}

void LiveTileData::parse() {
    if (state != State::loaded) {
        return;
    }

    try {
        if (!style) {
            throw std::runtime_error("style isn't present in LiveTileData object anymore");
        }

        mapbox::util::geojsonvt::Tile& in_tile = geojsonvt->getTile(id.z, id.x, id.y);

        if (in_tile) {
            // Parsing creates state that is encapsulated in TileParser. While parsing,
            // the TileParser object writes results into this objects. All other state
            // is going to be discarded afterwards.
            mapbox::util::geojsonvt::Tile* geojsonTile = &in_tile;
            LiveTile live_tile(geojsonTile);
            const LiveTile* lt = &live_tile;
            LiveTileParser parser(lt, *this, style, glyphAtlas, glyphStore, spriteAtlas, sprite);

            // Clear the style so that we don't have a cycle in the shared_ptr references.
            style.reset();

            parser.parse();
        } else {
            state = State::obsolete;
            return;
        }
    } catch (const std::exception& ex) {
        Log::Error(Event::ParseTile, "Live-parsing [%d/%d/%d] failed: %s", id.z, id.x, id.y, ex.what());
        state = State::obsolete;
        return;
    }

    if (state != State::obsolete) {
        state = State::parsed;
    }
}
