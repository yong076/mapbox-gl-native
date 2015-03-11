#ifndef MBGL_MAP_LIVE_TILE
#define MBGL_MAP_LIVE_TILE

#include <mbgl/map/geometry_tile.hpp>
#include <mbgl/util/geojsonvt.hpp>

namespace mbgl {

using TPoint = mapbox::util::geojsonvt::TilePoint;
using TRing = mapbox::util::geojsonvt::TileRing;
using TFeature = mapbox::util::geojsonvt::TileFeature;
using TTile = mapbox::util::geojsonvt::Tile;

class LiveTileFeature : public GeometryTileFeature {
public:
    LiveTileFeature(TFeature&);

    FeatureType getType() const override { return type; }
    mapbox::util::optional<Value> getValue(const std::string&) const override;
    GeometryCollection getGeometries() const override { return geometries; }

private:
    FeatureType type = FeatureType::Unknown;
    std::map<std::string, std::string> properties;
    GeometryCollection geometries;
};

class LiveTileLayer : public GeometryTileLayer {
public:
    LiveTileLayer(std::vector<TFeature>&);

    std::size_t featureCount() const override { return features.size(); }
    util::ptr<const GeometryTileFeature> feature(std::size_t i) const override { return features[i]; }

private:
    std::vector<util::ptr<const LiveTileFeature>> features;
};

class LiveTile : public GeometryTile {
public:
    LiveTile(TTile*);

    util::ptr<const GeometryTileLayer> getLayer(const std::string&) const override;

private:
    void convert();

private:
    TTile* tile;
    std::map<std::string, util::ptr<const LiveTileLayer>> layers;
};

}

#endif
