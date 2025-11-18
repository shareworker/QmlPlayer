#ifndef VIDEORENDERER_H
#define VIDEORENDERER_H

#include <QQuickFramebufferObject>
#include <QOpenGLFunctions>
#include <QObject>
#include <QString>

extern "C" {
#include <libavformat/avformat.h>
}

class VideoDecoder;
class GLVideoRenderer;

class VideoRenderer : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)

public:
    explicit VideoRenderer(QQuickItem *parent = nullptr);
    ~VideoRenderer();

    QString source() const;
    void setSource(const QString& source);

    Renderer *createRenderer() const override;
    void renderToFbo(QOpenGLFramebufferObject* fbo);

signals:
    void sourceChanged();

private:
    QString source_;
    VideoDecoder* decoder_;
    GLVideoRenderer* glRenderer_;
    
    friend class VideoRendererInternal;
};

class VideoRendererInternal : public QQuickFramebufferObject::Renderer {
public:
    VideoRendererInternal();
    ~VideoRendererInternal() override;

    void render() override;
    void synchronize(QQuickFramebufferObject *item) override;
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;

private:
    VideoRenderer* item_;
};

#endif // VIDEORENDERER_H
