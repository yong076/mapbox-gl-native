#ifndef MBGL_MAP_LIVE_TILE
#define MBGL_MAP_LIVE_TILE

#include <mbgl/map/geometry_tile.hpp>
#include <mbgl/util/geojsonvt.hpp>

namespace mbgl {

using TPoint = mapbox::util::geojsonvt::TilePoint;
using TRing = mapbox::util::geojsonvt::TileRing;
using TFeature = mapbox::util::geojsonvt::TileFeature;
using TTile = mapbox::util::geojsonvt::Tile;

class LiveTileFeature : public GeometryTileFeature, private util::noncopyable {
public:
    LiveTileFeature(FeatureType, GeometryCollection);
    LiveTileFeature(TFeature&);

    FeatureType getType() const override { return type; }
    mapbox::util::optional<Value> getValue(const std::string&) const override;
    GeometryCollection getGeometries() const override { return geometries; }

private:
    FeatureType type = FeatureType::Unknown;
    std::map<std::string, std::string> properties; // map to Values
    GeometryCollection geometries;
};

    class LiveTileLayer : public GeometryTileLayer, private util::noncopyable {
public:
    LiveTileLayer();
    LiveTileLayer(std::vector<TFeature>&);

    void prepareToAddFeatures(size_t count);
    void addFeature(util::ptr<const LiveTileFeature>);
    std::size_t featureCount() const override { return features.size(); }
    util::ptr<const GeometryTileFeature> feature(std::size_t i) const override { return features[i]; }

private:
    std::vector<util::ptr<const LiveTileFeature>> features;
};

class LiveTile : public GeometryTile, private util::noncopyable {
public:
    LiveTile();
    LiveTile(TTile*);

    void addLayer(const std::string&, util::ptr<const LiveTileLayer>);
    util::ptr<GeometryTileLayer> getLayer(const std::string&) const override;

private:
    void convert();

private:
    TTile* tile;
    std::map<std::string, util::ptr<LiveTileLayer>> layers;
};

}

#endif
