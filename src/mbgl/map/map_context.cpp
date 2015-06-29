#include <mbgl/map/map_context.hpp>
#include <mbgl/map/map_data.hpp>
#include <mbgl/map/view.hpp>
#include <mbgl/map/still_image.hpp>
#include <mbgl/map/annotation.hpp>

#include <mbgl/platform/log.hpp>

#include <mbgl/renderer/painter.hpp>

#include <mbgl/storage/file_source.hpp>
#include <mbgl/storage/resource.hpp>
#include <mbgl/storage/response.hpp>

#include <mbgl/style/style.hpp>
#include <mbgl/style/style_bucket.hpp>
#include <mbgl/style/style_layer.hpp>

#include <mbgl/util/gl_object_store.hpp>
#include <mbgl/util/uv_detail.hpp>
#include <mbgl/util/worker.hpp>
#include <mbgl/util/texture_pool.hpp>
#include <mbgl/util/exception.hpp>

#include <mbgl/shader/plain_shader.hpp>
#include <mbgl/util/box.hpp>

#include <algorithm>

namespace mbgl {

MapContext::MapContext(uv_loop_t* loop, View& view_, FileSource& fileSource, MapData& data_)
    : view(view_),
      data(data_),
      updated(static_cast<UpdateType>(Update::Nothing)),
      asyncUpdate(std::make_unique<uv::async>(loop, [this] { update(); })),
      texturePool(std::make_unique<TexturePool>()) {
    assert(util::ThreadContext::currentlyOn(util::ThreadType::Map));

    util::ThreadContext::setFileSource(&fileSource);
    util::ThreadContext::setGLObjectStore(&glObjectStore);

    asyncUpdate->unref();

    view.activate();
}

MapContext::~MapContext() {
    // Make sure we call cleanup() before deleting this object.
    assert(!style);
}

void MapContext::cleanup() {
    view.notify();

    // Explicit resets currently necessary because these abandon resources that need to be
    // cleaned up by glObjectStore.performCleanup();
    style.reset();
    painter.reset();
    texturePool.reset();

    glObjectStore.performCleanup();

    view.deactivate();
}

void MapContext::pause() {
    MBGL_CHECK_ERROR(glFinish());

    view.deactivate();

    std::unique_lock<std::mutex> lockPause(data.mutexPause);
    data.condPaused.notify_all();
    data.condResume.wait(lockPause);

    view.activate();
}

void MapContext::resize(uint16_t width, uint16_t height, float ratio) {
    view.resize(width, height, ratio);
}

void MapContext::triggerUpdate(const TransformState& state, const Update u) {
    transformState = state;
    updated |= static_cast<UpdateType>(u);

    asyncUpdate->send();
}

void MapContext::setStyleURL(const std::string& url) {
    styleURL = url;
    styleJSON.clear();

    const size_t pos = styleURL.rfind('/');
    std::string base = "";
    if (pos != std::string::npos) {
        base = styleURL.substr(0, pos + 1);
    }

    FileSource* fs = util::ThreadContext::getFileSource();
    fs->request({ Resource::Kind::Style, styleURL }, util::RunLoop::current.get()->get(), [this, base](const Response &res) {
        if (res.status == Response::Successful) {
            loadStyleJSON(res.data, base);
        } else {
            Log::Error(Event::Setup, "loading style failed: %s", res.message.c_str());
        }
    });
}

void MapContext::setStyleJSON(const std::string& json, const std::string& base) {
    styleURL.clear();
    styleJSON = json;

    loadStyleJSON(json, base);
}

void MapContext::loadStyleJSON(const std::string& json, const std::string& base) {
    assert(util::ThreadContext::currentlyOn(util::ThreadType::Map));

    style.reset();
    style = std::make_unique<Style>(json, base, asyncUpdate->get()->loop);
    style->cascade(data.getClasses());
    style->setDefaultTransitionDuration(data.getDefaultTransitionDuration());
    style->setObserver(this);

    updated |= static_cast<UpdateType>(Update::Zoom);
    asyncUpdate->send();

    auto staleTiles = data.annotationManager.resetStaleTiles();
    if (staleTiles.size()) {
        updateAnnotationTiles(staleTiles);
    }
}

void MapContext::updateAnnotationTiles(const std::unordered_set<TileID, TileID::Hash>& ids) {
    assert(util::ThreadContext::currentlyOn(util::ThreadType::Map));

    data.annotationManager.markStaleTiles(ids);

    if (!style) return;

    // grab existing, single shape annotations source
    const auto& shapeID = AnnotationManager::ShapeLayerID;
    style->getSource(shapeID)->enabled = true;

    // create (if necessary) layers and buckets for each shape
    for (const auto &shapeAnnotationID : data.annotationManager.getOrderedShapeAnnotations()) {
        const std::string shapeLayerID = shapeID + "." + std::to_string(shapeAnnotationID);

        const auto layer_it = std::find_if(style->layers.begin(), style->layers.end(),
            [&shapeLayerID](util::ptr<StyleLayer> layer) {
            return (layer->id == shapeLayerID);
        });

        if (layer_it == style->layers.end()) {
            // query shape styling
            auto& shapeStyle = data.annotationManager.getAnnotationStyleProperties(shapeAnnotationID);

            // apply shape paint properties
            ClassProperties paintProperties;

            if (shapeStyle.is<LineProperties>()) {
                // opacity
                PropertyValue lineOpacity = ConstantFunction<float>(shapeStyle.get<LineProperties>().opacity);
                paintProperties.set(PropertyKey::LineOpacity, lineOpacity);

                // line width
                PropertyValue lineWidth = ConstantFunction<float>(shapeStyle.get<LineProperties>().width);
                paintProperties.set(PropertyKey::LineWidth, lineWidth);

                // stroke color
                PropertyValue strokeColor = ConstantFunction<Color>(shapeStyle.get<LineProperties>().color);
                paintProperties.set(PropertyKey::LineColor, strokeColor);
            } else if (shapeStyle.is<FillProperties>()) {
                // opacity
                PropertyValue fillOpacity = ConstantFunction<float>(shapeStyle.get<FillProperties>().opacity);
                paintProperties.set(PropertyKey::FillOpacity, fillOpacity);

                // fill color
                PropertyValue fillColor = ConstantFunction<Color>(shapeStyle.get<FillProperties>().fill_color);
                paintProperties.set(PropertyKey::FillColor, fillColor);

                // stroke color
                PropertyValue strokeColor = ConstantFunction<Color>(shapeStyle.get<FillProperties>().stroke_color);
                paintProperties.set(PropertyKey::FillOutlineColor, strokeColor);
            }

            std::map<ClassID, ClassProperties> shapePaints;
            shapePaints.emplace(ClassID::Default, std::move(paintProperties));

            // create shape layer
            util::ptr<StyleLayer> shapeLayer = std::make_shared<StyleLayer>(shapeLayerID, std::move(shapePaints));
            shapeLayer->type = (shapeStyle.is<LineProperties>() ? StyleLayerType::Line : StyleLayerType::Fill);

            // add to end of other shape layers just before (last) point layer
            style->layers.emplace((style->layers.end() - 2), shapeLayer);

            // create shape bucket & connect to source
            util::ptr<StyleBucket> shapeBucket = std::make_shared<StyleBucket>(shapeLayer->type);
            shapeBucket->name = shapeLayer->id;
            shapeBucket->source = shapeID;
            shapeBucket->source_layer = shapeLayer->id;

            // apply line layout properties to bucket
            if (shapeStyle.is<LineProperties>()) {
                shapeBucket->layout.set(PropertyKey::LineJoin, ConstantFunction<JoinType>(JoinType::Round));
            }

            // connect layer to bucket
            shapeLayer->bucket = shapeBucket;
        }
    }

    // invalidate annotations layer tiles
    for (const auto &source : style->sources) {
        if (source->info.type == SourceType::Annotations) {
            source->invalidateTiles(ids);
        }
    }

    cascadeClasses();

    updated |= static_cast<UpdateType>(Update::Classes);
    asyncUpdate->send();

    data.annotationManager.resetStaleTiles();
}

void MapContext::cascadeClasses() {
    style->cascade(data.getClasses());
}

void MapContext::update() {
    assert(util::ThreadContext::currentlyOn(util::ThreadType::Map));

    const auto now = Clock::now();
    data.setAnimationTime(now);

    if (style) {
        if (updated & static_cast<UpdateType>(Update::DefaultTransitionDuration)) {
            style->setDefaultTransitionDuration(data.getDefaultTransitionDuration());
        }

        if (updated & static_cast<UpdateType>(Update::Classes)) {
            cascadeClasses();
        }

        if (updated & static_cast<UpdateType>(Update::Classes) ||
            updated & static_cast<UpdateType>(Update::Zoom)) {
            style->recalculate(transformState.getNormalizedZoom(), now);
        }

        style->update(data, transformState, *texturePool);

        if (style->isLoaded()) {
            if (!data.getFullyLoaded()) {
                data.setFullyLoaded(true);
            }
        } else {
            if (data.getFullyLoaded()) {
                data.setFullyLoaded(false);
            }
        }

        if (callback) {
            renderSync(transformState);
        } else {
            view.invalidate();
        }
    }

    updated = static_cast<UpdateType>(Update::Nothing);
}

void MapContext::renderStill(const TransformState& state, StillImageCallback fn) {
    if (!fn) {
        Log::Error(Event::General, "StillImageCallback not set");
        return;
    }

    if (data.mode != MapMode::Still) {
        fn(std::make_exception_ptr(util::MisuseException("Map is not in still image render mode")), nullptr);
        return;
    }

    if (callback) {
        fn(std::make_exception_ptr(util::MisuseException("Map is currently rendering an image")), nullptr);
        return;
    }

    if (!style) {
        fn(std::make_exception_ptr(util::MisuseException("Map doesn't have a style")), nullptr);
        return;
    }

    if (style->getLastError()) {
        fn(style->getLastError(), nullptr);
        return;
    }

    callback = fn;
    transformState = state;

    updated |= static_cast<UpdateType>(Update::RenderStill);
    asyncUpdate->send();
}

bool MapContext::renderSync(const TransformState& state) {
    assert(util::ThreadContext::currentlyOn(util::ThreadType::Map));

    transformState = state;

    // Cleanup OpenGL objects that we abandoned since the last render call.
    glObjectStore.performCleanup();

    if (data.mode == MapMode::Still && (!callback || !data.getFullyLoaded())) {
        // We are either not waiting for a map to be rendered, or we don't have all resources yet.
        return false;
    }

    assert(style);

    if (!painter) {
        painter = std::make_unique<Painter>();
        painter->setup();
    }

    painter->setDebug(data.getDebug());
    painter->render(*style, transformState, data.getAnimationTime());






    const auto s = data.transform.currentState();

    const auto l = LatLng(38.87031006108791, -77.00896639534831);

    vec2<double> p = s.pixelForLatLng(l);

//    // flip Y to match GL
//    p.y = s.getHeight() - p.y;

    // create a GL coordinate system billboard rect
    double size = 20;

    box b;
    b.tl.x = (p.x - size / 2) / s.getWidth();
    b.tl.y = (p.y - size / 2) / s.getHeight();

    b.tr.x = b.tl.x + size / s.getWidth();
    b.tr.y = b.tl.y;

    b.bl.x = b.tl.x;
    b.bl.y = b.tl.y + size / s.getHeight();

    b.br.x = b.tr.x;
    b.br.y = b.bl.y;

    b.tl.x = b.tl.x * 2 - 1 + size / 2 / s.getWidth();
    b.tr.x = b.tr.x * 2 - 1 + size / 2 / s.getWidth();
    b.bl.x = b.bl.x * 2 - 1 + size / 2 / s.getWidth();
    b.br.x = b.br.x * 2 - 1 + size / 2 / s.getWidth();

    b.tl.y = b.tl.y * 2 - 1 + size / 2 / s.getHeight();
    b.tr.y = b.tr.y * 2 - 1 + size / 2 / s.getHeight();
    b.bl.y = b.bl.y * 2 - 1 + size / 2 / s.getHeight();
    b.br.y = b.br.y * 2 - 1 + size / 2 / s.getHeight();

    GLfloat vertices[] =
    {
        (GLfloat)b.tl.x, (GLfloat)b.tl.y,
        (GLfloat)b.bl.x, (GLfloat)b.bl.y,
        (GLfloat)b.br.x, (GLfloat)b.br.y,
        (GLfloat)b.br.x, (GLfloat)b.br.y,
        (GLfloat)b.tr.x, (GLfloat)b.tr.y,
        (GLfloat)b.tl.x, (GLfloat)b.tl.y

//        0, 0,
//        0, 1,
//        1, 1
    };

//    (void)vertices;
//    (void)vbo;
//    (void)vao;

//    // hack through a basic shading of the billboard
    glGenVertexArraysOES(1, &vao);
    glBindVertexArrayOES(vao);

    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

//    const GLchar* vertexSource =
//    "attribute vec2 position;"
//    "void main() {"
//    "   gl_Position = vec4(position, 0.0, 1.0);"
//    "}";
//    const GLchar* fragmentSource =
//    "void main() {"
//    "   gl_FragColor = vec4(0, 1.0, 0, 1.0);"
//    "}";
//
//    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
//    glShaderSource(vertexShader, 1, &vertexSource, NULL);
//    MBGL_CHECK_ERROR(glCompileShader(vertexShader));
//
//    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
//    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
//    MBGL_CHECK_ERROR(glCompileShader(fragmentShader));
//
//    GLuint shaderProgram = glCreateProgram();
//    MBGL_CHECK_ERROR(glAttachShader(shaderProgram, vertexShader));
//    MBGL_CHECK_ERROR(glAttachShader(shaderProgram, fragmentShader));
//    MBGL_CHECK_ERROR(glLinkProgram(shaderProgram));
//    MBGL_CHECK_ERROR(glUseProgram(shaderProgram));

    painter->useProgram(painter->plainShader->program);

    painter->plainShader->u_matrix = painter->identityMatrix;
    painter->plainShader->u_color = {{ 0.0f, 1.0f, 0.0f, 1.0f }};

    GLint posAttrib = glGetAttribLocation(painter->plainShader->program, "a_pos");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0, sizeof(vertices) / sizeof(GLfloat));





    if (data.mode == MapMode::Still) {
        callback(nullptr, view.readStillImage());
        callback = nullptr;
    }

    view.swap();

    return style->hasTransitions();
}

double MapContext::getTopOffsetPixelsForAnnotationSymbol(const std::string& symbol) {
    assert(util::ThreadContext::currentlyOn(util::ThreadType::Map));
    const SpritePosition pos = style->sprite->getSpritePosition(symbol);
    return -pos.height / pos.pixelRatio / 2;
}

void MapContext::setSourceTileCacheSize(size_t size) {
    assert(util::ThreadContext::currentlyOn(util::ThreadType::Map));
    if (size != sourceCacheSize) {
        sourceCacheSize = size;
        if (!style) return;
        for (const auto &source : style->sources) {
            source->setCacheSize(sourceCacheSize);
        }
        view.invalidate();
    }
}

void MapContext::onLowMemory() {
    assert(util::ThreadContext::currentlyOn(util::ThreadType::Map));
    if (!style) return;
    for (const auto &source : style->sources) {
        source->onLowMemory();
    }
    view.invalidate();
}

void MapContext::onTileDataChanged() {
    assert(util::ThreadContext::currentlyOn(util::ThreadType::Map));
    asyncUpdate->send();
}

void MapContext::onResourceLoadingFailed(std::exception_ptr error) {
    assert(util::ThreadContext::currentlyOn(util::ThreadType::Map));

    if (data.mode == MapMode::Still && callback) {
        callback(error, nullptr);
        callback = nullptr;
    }
}

}
