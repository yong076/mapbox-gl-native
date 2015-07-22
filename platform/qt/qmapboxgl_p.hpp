#ifndef QMAPBOXGL_P_H
#define QMAPBOXGL_P_H

#include <mbgl/map/map.hpp>
#include <mbgl/map/view.hpp>
#include <mbgl/storage/default_file_source.hpp>

#include <QObject>

namespace mbgl {

class Map;
class FileSource;

}  // namespace mbgl

class QGLContext;
class QMapboxGL;

class QMapboxGLPrivate : public QObject, public mbgl::View
{
    Q_OBJECT

public:
    explicit QMapboxGLPrivate(QMapboxGL *q);
    virtual ~QMapboxGLPrivate();

    // mbgl::View implementation.
    float getPixelRatio() const override;
    std::array<uint16_t, 2> getSize() const override;
    std::array<uint16_t, 2> getFramebufferSize() const override;
    void activate() override;
    void deactivate() override;
    void notify() override;
    void invalidate() override;
    void swap() override;

    std::array<uint16_t, 2> size;

    bool contextIsCurrent;
    QGLContext *context;

    mbgl::DefaultFileSource fileSourceObj;
    mbgl::Map mapObj;

    QMapboxGL *q_ptr;

signals:
    void viewInvalidated();

public slots:
    void triggerRender();
};

#endif // QMAPBOXGL_P_H
