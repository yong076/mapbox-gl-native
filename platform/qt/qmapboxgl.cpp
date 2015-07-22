#include "qmapboxgl_p.hpp"

#include <mbgl/map/map.hpp>
#include <mbgl/platform/gl.hpp>
#include <mbgl/platform/qt/qmapboxgl.hpp>
#include <mbgl/storage/default_file_source.hpp>

#include <QGLContext>
#include <QString>

QMapboxGL::QMapboxGL(QGLContext *context) : d_ptr(new QMapboxGLPrivate(this))
{
    d_ptr->context = context;
}

QMapboxGL::~QMapboxGL()
{
    delete d_ptr;
}

void QMapboxGL::setAccessToken(const QString &token)
{
    d_ptr->fileSourceObj.setAccessToken(token.toUtf8().constData());
}

void QMapboxGL::setStyleJSON(const QString &style)
{
    d_ptr->mapObj.setStyleJSON(style.toUtf8().constData());
}

void QMapboxGL::setStyleURL(const QString &url)
{
    d_ptr->mapObj.setStyleURL(url.toUtf8().constData());
}

double QMapboxGL::latitude() const
{
    return d_ptr->mapObj.getLatLng().latitude;
}

void QMapboxGL::setLatitude(double latitude_)
{
    d_ptr->mapObj.setLatLng({ latitude_, longitude() });
}

double QMapboxGL::longitude() const
{
    return d_ptr->mapObj.getLatLng().longitude;
}

void QMapboxGL::setLongitude(double longitude_)
{
    d_ptr->mapObj.setLatLng({ latitude(), longitude_ });
}

double QMapboxGL::zoom() const
{
    return d_ptr->mapObj.getZoom();
}

void QMapboxGL::setZoom(double zoom_)
{
    d_ptr->mapObj.setZoom(zoom_);
}

void QMapboxGL::moveBy(double dx, double dy) {
    d_ptr->mapObj.moveBy(dx, dy);
}

void QMapboxGL::scaleBy(double ds, double cx, double cy) {
    d_ptr->mapObj.scaleBy(ds, cx, cy);
}

void QMapboxGL::resize(int w, int h)
{
    d_ptr->size = {{ static_cast<uint16_t>(w), static_cast<uint16_t>(h) }};
    d_ptr->mapObj.update(mbgl::Update::Dimensions);
}

QMapboxGLPrivate::QMapboxGLPrivate(QMapboxGL *q)
    : contextIsCurrent(false)
    , fileSourceObj(nullptr)
    , mapObj(*this, fileSourceObj)
    , q_ptr(q)
{
    connect(this, SIGNAL(viewInvalidated(void)), this, SLOT(triggerRender()), Qt::QueuedConnection);
}

QMapboxGLPrivate::~QMapboxGLPrivate()
{
}

float QMapboxGLPrivate::getPixelRatio() const
{
    return 1.0;
}

std::array<uint16_t, 2> QMapboxGLPrivate::getSize() const
{
    return size;
}

std::array<uint16_t, 2> QMapboxGLPrivate::getFramebufferSize() const
{
    return size;
}

void QMapboxGLPrivate::activate()
{
    // Map thread
}

void QMapboxGLPrivate::deactivate()
{
    // Map thread
    context->doneCurrent();
}

void QMapboxGLPrivate::notify()
{
    // Map thread
}

void QMapboxGLPrivate::invalidate()
{
    // Map thread
    if (!context->isValid()) {
        return;
    }

    if (!contextIsCurrent) {
        context->makeCurrent();
        contextIsCurrent = true;

        mbgl::gl::InitializeExtensions([](const char *name) {
            const QGLContext *thisContext = QGLContext::currentContext();
            return reinterpret_cast<mbgl::gl::glProc>(thisContext->getProcAddress(name));
        });
    }

    emit viewInvalidated();
}

void QMapboxGLPrivate::swap()
{
    // Map thread
    context->swapBuffers();
}

void QMapboxGLPrivate::triggerRender() {
    mapObj.renderSync();
}
