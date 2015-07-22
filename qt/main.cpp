#include "../platform/default/default_styles.hpp"

#include <mbgl/platform/qt/QMapboxGL>

#include <QApplication>
#include <QGLContext>
#include <QGLFormat>
#include <QGLWidget>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QString>

class MapWindow : public QGLWidget {
public:
    MapWindow(QGLContext *ctx)
        : QGLWidget(ctx) // QGLWidget takes the ownership of ctx.
        , m_map(ctx)
    {
        m_map.setAccessToken(qgetenv("MAPBOX_ACCESS_TOKEN"));
        changeStyle();
    }

private:
    void changeStyle()
    {
        static uint8_t currentStyleIndex;

        const auto& newStyle = mbgl::util::defaultStyles[currentStyleIndex];
        QString url(newStyle.first.c_str());

        m_map.setStyleURL(url);

        QString name(newStyle.second.c_str());
        setWindowTitle(QString("Mapbox GL: ") + name);

        if (++currentStyleIndex == mbgl::util::defaultStyles.size()) {
            currentStyleIndex = 0;
        }
    }

    void keyPressEvent(QKeyEvent *ev) override
    {
        if (ev->key() == Qt::Key_S) {
            changeStyle();
        }

        ev->accept();
    }

    void mousePressEvent(QMouseEvent *ev) override
    {
        if (!(ev->buttons() & Qt::LeftButton)) {
            return;
        }

        lastX = ev->x();
        lastY = ev->y();

        ev->accept();
    }

    void mouseMoveEvent(QMouseEvent *ev) override
    {
        if (!(ev->buttons() & Qt::LeftButton)) {
            return;
        }

        int dx = ev->x() - lastX;
        int dy = ev->y() - lastY;

        lastX = ev->x();
        lastY = ev->y();

        if (dx || dy) {
            m_map.moveBy(dx, dy);
        }

        ev->accept();
    }

    void wheelEvent(QWheelEvent *ev) override
    {
        int numDegrees = ev->delta() / 8;

        if (numDegrees > 0) {
            m_map.scaleBy(1.10, ev->x(), ev->y());
        } else {
            m_map.scaleBy(0.90, ev->x(), ev->y());
        }

        ev->accept();
    }

    void resizeGL(int w, int h) override
    {
        m_map.resize(w, h);
    }

    int lastX = 0;
    int lastY = 0;

    QMapboxGL m_map;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QGLContext *context = new QGLContext(QGLFormat());
    context->create();

    // MapWindow inherits from QGLWidget and will get
    // the ownership of the QGLContext while internally
    // it will create a QMapboxGL that will use the
    // context for rendering. QGLWidget will also handle
    // the input events and forward them to QMapboxGL
    // if necessary.
    MapWindow view(context);

    view.resize(800, 600);
    view.setFocus();
    view.show();

    return app.exec();
}
