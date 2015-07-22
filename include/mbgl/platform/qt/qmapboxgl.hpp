#ifndef QMAPBOXGL_H
#define QMAPBOXGL_H

#include <QObject>

class QGLContext;
class QString;

class QMapboxGLPrivate;

class QMapboxGL : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double latitude READ latitude WRITE setLatitude)
    Q_PROPERTY(double longitude READ longitude WRITE setLongitude)
    Q_PROPERTY(double zoom READ zoom WRITE setZoom)

public:
    QMapboxGL(QGLContext *context);
    ~QMapboxGL();

    void setAccessToken(const QString &token);

    void setStyleJSON(const QString &style);
    void setStyleURL(const QString &url);

    double latitude() const;
    void setLatitude(double latitude);

    double longitude() const;
    void setLongitude(double longitude);

    double zoom() const;
    void setZoom(double zoom);

    void moveBy(double dx, double dy);
    void scaleBy(double ds, double cx, double cy);

    void resize(int w, int h);

private:
    Q_DISABLE_COPY(QMapboxGL)

    QMapboxGLPrivate *d_ptr;
};

#endif // QMAPBOXGL_H
