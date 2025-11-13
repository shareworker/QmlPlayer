#ifndef VIDEORENDERER_H
#define VIDEORENDERER_H

#include <QQuickFramebufferObject>
#include <QSGSimpleRectNode>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_3_Core>
#include <QObject>
#include <QString>

extern "C" {
#include <libavformat/avformat.h>
}

class VideoDecoder;

class VideoRenderer : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)

public:
    explicit VideoRenderer(QQuickItem *parent = nullptr);
    ~VideoRenderer();

    QString source() const;
    void setSource(const QString& source);

    Renderer *createRenderer() const override;

signals:
    void sourceChanged();

private:
    QString source_;
    VideoDecoder* decoder_;
    
    friend class VideoRendererInternal;
};

class VideoRendererInternal : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions_3_3_Core {
public:
    VideoRendererInternal();
    ~VideoRendererInternal();

    void render() override;
    void synchronize(QQuickFramebufferObject *item) override;
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;

private:
    void initShaders();
    void updateTextures(AVFrame* frame);

    GLuint textureY_;
    GLuint textureU_;
    GLuint textureV_;
    GLuint shaderProgram_;
    GLuint vertexArray_;
    VideoDecoder* decoder_;
};

#endif // VIDEORENDERER_H
