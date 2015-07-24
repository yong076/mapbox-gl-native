#ifndef MBGL_STORAGE_IOS_FILE_SOURCE
#define MBGL_STORAGE_IOS_FILE_SOURCE

#include <mbgl/storage/default_file_source.hpp>

@class MGLMapView;

namespace mbgl {

class iOSFileSource : public DefaultFileSource {
public:
    iOSFileSource(MGLMapView*, FileCache*, const std::string &root = "");

    Request* request(const Resource&, uv_loop_t*, Callback);
    void cancel(Request*) override;

private:
    bool cancelled;
};

}

#endif
