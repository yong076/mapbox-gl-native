#ifndef MBGL_MAP_LIVE_TILE_PARSER
#define MBGL_MAP_LIVE_TILE_PARSER

#include <mbgl/style/filter_expression.hpp>
#include <mbgl/style/class_properties.hpp>
#include <mbgl/style/style_bucket.hpp>
#include <mbgl/text/glyph.hpp>
#include <mbgl/util/ptr.hpp>
#include <mbgl/util/noncopyable.hpp>
#include <mbgl/util/geojsonvt.hpp>

#include <cstdint>
#include <iosfwd>
#include <string>

namespace mbgl {

class Bucket;
class FontStack;
class GlyphAtlas;
class GlyphStore;
class SpriteAtlas;
class Sprite;
class Style;
class StyleBucket;
class StyleLayoutFill;
class StyleLayoutRaster;
class StyleLayoutLine;
class StyleLayoutSymbol;
class StyleLayerGroup;
class LiveTileData;
class Collision;

class LiveTileParser : private util::noncopyable
{
public:
    LiveTileParser(mapbox::util::geojsonvt::Tile& in_tile, LiveTileData &tile,
                   const util::ptr<const Style> &style/*,
                   GlyphAtlas & glyphAtlas,
                   GlyphStore & glyphStore,
                   SpriteAtlas & spriteAtlas,
                   const util::ptr<Sprite> &sprite*/);
    ~LiveTileParser();

public:
    void parse();

private:
    bool obsolete() const;
    void parseStyleLayers(util::ptr<const StyleLayerGroup> group);

    std::unique_ptr<Bucket> createBucket(const StyleBucket &bucket_desc);
//    std::unique_ptr<Bucket> createFillBucket(const mapbox::util::geojsonvt::TileFeature &feature, const StyleBucket &bucket_desc);
    std::unique_ptr<Bucket> createLineBucket(const mapbox::util::geojsonvt::Tile &tile, const StyleBucket &bucket_desc);
//    std::unique_ptr<Bucket> createSymbolBucket(const mapbox::util::geojsonvt::TileFeature &feature, const StyleBucket &bucket_desc);

    template <class Bucket> void addBucketGeometries(Bucket& bucket, const mapbox::util::geojsonvt::Tile &tile);

private:
    mapbox::util::geojsonvt::Tile& in_tile;
    LiveTileData& tile;

    // Cross-thread shared data.
    util::ptr<const Style> style;
//    GlyphAtlas & glyphAtlas;
//    GlyphStore & glyphStore;
//    SpriteAtlas & spriteAtlas;
//    util::ptr<Sprite> sprite;

    std::unique_ptr<Collision> collision;
};
    
}

#endif
