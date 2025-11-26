#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQml/qqml.h>
#include <QJSEngine>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include "core/VideoRenderer.h"
#include "core/ConfigBridge.h"

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

    QCoreApplication::setOrganizationName(QStringLiteral("QmlPlayer"));
    QCoreApplication::setApplicationName(QStringLiteral("QMLPlayerFFmpeg"));

    QGuiApplication app(argc, argv);
    qmlRegisterType<VideoRenderer>("VideoPlayer", 1, 0, "VideoRenderer");
    qmlRegisterSingletonType<ConfigBridge>("VideoPlayer", 1, 0, "Config",
        [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
            Q_UNUSED(engine)
            Q_UNUSED(scriptEngine)
            return new ConfigBridge();
        });

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}