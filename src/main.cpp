#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQml/qqml.h>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include "core/VideoRenderer.h"

int main(int argc, char *argv[]) {
    // Configure OpenGL surface format (no explicit version/profile)
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);

    // Force the OpenGL scene graph backend
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    // Use a QML-based style so control customization (background/handle) is supported
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    QGuiApplication app(argc, argv);
    qmlRegisterType<VideoRenderer>("VideoPlayer", 1, 0, "VideoRenderer");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}