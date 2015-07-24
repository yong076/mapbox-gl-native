#include "ios_file_source.h"

#include <mbgl/storage/request.hpp>
#include <mbgl/util/run_loop.hpp>

#import "MGLMapView.h"

#import <Foundation/Foundation.h>

@interface MGLFileSource: NSObject

@property (nonatomic, weak) MGLMapView *mapView;

+ (void)registerMapView:(MGLMapView *)mapView;
+ (MGLMapView *)registeredMapView;

@end

@implementation MGLFileSource

+ (instancetype)sharedInstance {
    static dispatch_once_t onceToken;
    static MGLFileSource *_sharedInstance;
    dispatch_once(&onceToken, ^{
        _sharedInstance = [[self alloc] init];
    });
    return _sharedInstance;
}

+ (void)registerMapView:(MGLMapView *)mapView {
    [[MGLFileSource sharedInstance] setMapView:mapView];
}

+ (MGLMapView *)registeredMapView {
    return [[MGLFileSource sharedInstance] mapView];
}

@end

namespace mbgl {

iOSFileSource::iOSFileSource(MGLMapView *mapView, FileCache *cache, const std::string &root)
    : DefaultFileSource(cache, root) {
    [MGLFileSource registerMapView:mapView];
}

Request* iOSFileSource::request(const Resource& resource,
                                uv_loop_t* loop,
                                Callback callback) {
    MGLMapView *mapView = [MGLFileSource registeredMapView];

    if (resource.kind == Resource::Kind::Tile && [mapView.delegate respondsToSelector:@selector(mapView:dataForTileURL:)]) {

        NSString *aString = [[NSString alloc] initWithCString:resource.url.c_str() encoding:NSUTF8StringEncoding];
        NSURL *aURL = [NSURL URLWithString:aString];

        auto req = new Request({ resource.kind, resource.url }, loop, std::move(callback));

//        auto runLoop = util::RunLoop::Get();

        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            NSData *aData = [mapView.delegate mapView:mapView dataForTileURL:aURL];

//            if (cancelled) {
//                NSLog(@"cancelling %@", aURL);
//                return;
//            }

            Response res;

            if (aData) {
                res.status = Response::Status::Successful;
                res.data = { (const char *)aData.bytes, aData.length };
            }

//            if (req->resource.url == "" || req->resource.kind != Resource::Kind::Tile) return;

            req->notify(std::make_unique<Response>(res));

//            runLoop->invoke(&Request::notify); //, std::make_unique<Response>(res));

        });

        return req;
    }

    return DefaultFileSource::request(resource, loop, callback);
}

void iOSFileSource::cancel(Request *req) {
    cancelled = true;
    DefaultFileSource::cancel(req);
}

}
