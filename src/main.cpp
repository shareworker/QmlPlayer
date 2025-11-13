#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQml/qqml.h>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include "core/VideoRenderer.h"

int main(int argc, char *argv[]) {
    // Configure OpenGL surface format
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);
    
    // Force the OpenGL scene graph backend
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    
    QGuiApplication app(argc, argv);
    qmlRegisterType<VideoRenderer>("VideoPlayer", 1, 0, "VideoRenderer");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}