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
    Q_PROPERTY(int state READ state NOTIFY stateChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)

public:
    explicit VideoRenderer(QQuickItem *parent = nullptr);
    ~VideoRenderer();

    QString source() const;
    void setSource(const QString& source);

    Renderer *createRenderer() const override;
    void renderToFbo(QOpenGLFramebufferObject* fbo);

    int state() const;
    qint64 duration() const;
    qint64 position() const;

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void seek(qint64 position);

signals:
    void sourceChanged();
    void stateChanged(int state);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);

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
