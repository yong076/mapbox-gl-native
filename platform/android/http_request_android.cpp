#include <mbgl/storage/http_context_base.hpp>
#include <mbgl/storage/http_request_base.hpp>
#include <mbgl/storage/resource.hpp>
#include <mbgl/storage/response.hpp>
#include <mbgl/util/chrono.hpp>
#include <mbgl/platform/log.hpp>

#include <mbgl/util/time.hpp>
#include <mbgl/util/util.hpp>

namespace mbgl {

class HTTPAndroidRequest;

class HTTPAndroidContext : public HTTPContextBase {
public:
    explicit HTTPAndroidContext(uv_loop_t *loop);
    ~HTTPAndroidContext();

    HTTPRequestBase* createRequest(const Resource&,
                               RequestBase::Callback,
                               uv_loop_t*,
                               std::shared_ptr<const Response>) final;

    uv_loop_t *loop = nullptr;

    // TODO permanent java handle to the java singleton
};

class HTTPAndroidRequest : public HTTPRequestBase {
public:
    HTTPAndroidRequest(HTTPAndroidContext*,
                const Resource&,
                Callback,
                uv_loop_t*,
                std::shared_ptr<const Response>);
    ~HTTPAndroidRequest();

    void cancel() final;
    void retry() final;

    void handleResult();

private:
    void retry(uint64_t timeout) final;
#if UV_VERSION_MAJOR == 0 && UV_VERSION_MINOR <= 10
    static void restart(uv_timer_t *timer, int);
#else
    static void restart(uv_timer_t *timer);
#endif
    void finish(ResponseStatus status);
    void start();

    HTTPAndroidContext *context = nullptr;

    std::unique_ptr<Response> response;
    const std::shared_ptr<const Response> existingResponse;

    // TODO permanent java handle to the java request

    uv_timer_t *timer = nullptr;
    enum : bool { PreemptImmediately, ExponentialBackoff } strategy = PreemptImmediately;
    int attempts = 0;

    static const int maxAttempts = 4;
};

// -------------------------------------------------------------------------------------------------

HTTPAndroidContext::HTTPAndroidContext(uv_loop_t *loop_)
    : HTTPContextBase(loop_),
      loop(loop_) {
    // TODO create java singleton
}

HTTPAndroidContext::~HTTPAndroidContext() {
    // TODO delete java singleton
}

HTTPRequestBase* HTTPAndroidContext::createRequest(const Resource& resource,
                                            RequestBase::Callback callback,
                                            uv_loop_t* loop_,
                                            std::shared_ptr<const Response> response) {
    return new HTTPAndroidRequest(this, resource, callback, loop_, response);
}

HTTPAndroidRequest::HTTPAndroidRequest(HTTPAndroidContext* context_, const Resource& resource_, Callback callback_, uv_loop_t*, std::shared_ptr<const Response> response_)
    : HTTPRequestBase(resource_, callback_),
      context(context_),
      existingResponse(response_) {

    // TODO 304 stuff

    // TODO create the java request object

    start();
}

HTTPAndroidRequest::~HTTPAndroidRequest() {
    context->removeRequest(this);

    // TODO delete the java request object

    if (timer) {
        uv_timer_stop(timer);
        uv::close(timer);
        timer = nullptr;
    }
}

void HTTPAndroidRequest::cancel() {
   delete this;
}

void HTTPAndroidRequest::start() {
    attempts++;

    // TODO queue the request
}

void HTTPAndroidRequest::retry(uint64_t timeout) {
    // TODO cancel the request?

    response.reset();

    assert(!timer);
    timer = new uv_timer_t;
    timer->data = this;
    uv_timer_init(context->loop, timer);
    uv_timer_start(timer, restart, timeout, 0);
}

void HTTPAndroidRequest::retry() {
    if (timer && strategy == PreemptImmediately) {
        uv_timer_stop(timer);
        uv_timer_start(timer, restart, 0, 0);
    }
}

#if UV_VERSION_MAJOR == 0 && UV_VERSION_MINOR <= 10
void HTTPAndroidRequest::restart(uv_timer_t *timer, int) {
#else
void HTTPAndroidRequest::restart(uv_timer_t *timer) {
#endif
    auto baton = reinterpret_cast<HTTPAndroidRequest *>(timer->data);

    baton->timer = nullptr;
    uv::close(timer);

    baton->start();
}

void HTTPAndroidRequest::finish(ResponseStatus status) {
    if (status == ResponseStatus::TemporaryError && attempts < maxAttempts) {
        strategy = ExponentialBackoff;
        return retry((1 << (attempts - 1)) * 1000);
    } else if (status == ResponseStatus::ConnectionError && attempts < maxAttempts) {
        strategy = PreemptImmediately;
        return retry(30000);
    }

    if (status == ResponseStatus::NotModified) {
        notify(std::move(response), FileCache::Hint::Refresh);
    } else {
        notify(std::move(response), FileCache::Hint::Full);
    }

    delete this;
}

void HTTPAndroidRequest::handleResult() {
    if (cancelled) {
        delete this;
        return;
    }

    if (!response) {
        response = std::make_unique<Response>();
    }

    // TODO parse response code
    // fill out response object
    // call finish() with correct status
    // parse headers (in Java?)

    throw std::runtime_error("Response hasn't been handled");
}

std::unique_ptr<HTTPContextBase> HTTPContextBase::createContext(uv_loop_t* loop) {
    return std::make_unique<HTTPAndroidContext>(loop);
}

}
