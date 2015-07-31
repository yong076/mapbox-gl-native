#ifndef MBGL_MAP_CAMERA
#define MBGL_MAP_CAMERA

#include <mbgl/util/geo.hpp>
#include <mbgl/util/optional.hpp>

namespace mbgl {

struct CameraOptions {
    mapbox::util::optional<LatLng> center;
    mapbox::util::optional<double> zoom;
    mapbox::util::optional<double> angle;
};

}

#endif /* MBGL_MAP_CAMERA */
