#include <mbgl/map/live_tile_data.hpp>
#include <mbgl/map/live_tile_parser.hpp>
#include <mbgl/style/style_layer.hpp>
#include <mbgl/style/style_bucket.hpp>
#include <mbgl/geometry/glyph_atlas.hpp>
#include <mbgl/platform/log.hpp>

using namespace mbgl;

LiveTileData::LiveTileData(Tile::ID const& id_,
                           float mapMaxZoom, util::ptr<Style> style_,
                           GlyphAtlas& glyphAtlas_, GlyphStore& glyphStore_,
                           SpriteAtlas& spriteAtlas_, util::ptr<Sprite> sprite_,
                           const SourceInfo& source_, FileSource &fileSource_,
                           util::ptr<mapbox::util::geojsonvt::GeoJSONVT>& geojsonvt_)
    : TileData(id_, source_, fileSource_),
      glyphAtlas(glyphAtlas_),
      glyphStore(glyphStore_),
      spriteAtlas(spriteAtlas_),
      sprite(sprite_),
      style(style_),
      geojsonvt(geojsonvt_),
      depth(id.z >= source.max_zoom ? mapMaxZoom - id.z : 1) {

    // GeoJSON is already loaded by now
    state = State::loaded;
}

LiveTileData::~LiveTileData() {
    glyphAtlas.removeGlyphs(id.to_uint64());
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
            printf(" + translating tile [%d/%d/%d]\n", id.z, id.x, id.y);

            LiveTileParser parser(in_tile, *this, style/*, glyphAtlas, glyphStore, spriteAtlas, sprite*/);
            style.reset();

            parser.parse();
        } else {
            printf(" - not a valid tile [%d/%d/%d]\n", id.z, id.x, id.y);

            state = State::obsolete;
            return;
        }
    } catch (const std::exception& ex) {
        Log::Error(Event::ParseTile, "Live-converting [%d/%d/%d] failed: %s", id.z, id.x, id.y, ex.what());
        state = State::obsolete;
        return;
    }

    if (state != State::obsolete) {
        state = State::parsed;
    }
}

void LiveTileData::render(Painter &painter, util::ptr<StyleLayer> layer_desc, const mat4 &matrix) {
    if (state == State::parsed && layer_desc->bucket) {
        auto databucket_it = buckets.find(layer_desc->bucket->name);
        if (databucket_it != buckets.end()) {
            assert(databucket_it->second);
            databucket_it->second->render(painter, layer_desc, id, matrix);
        }
    }
}

bool LiveTileData::hasData(StyleLayer const& layer_desc) const {
    if (state == State::parsed && layer_desc.bucket) {
        auto databucket_it = buckets.find(layer_desc.bucket->name);
        if (databucket_it != buckets.end()) {
            assert(databucket_it->second);
            return databucket_it->second->hasData();
        }
    }
    return false;
}
