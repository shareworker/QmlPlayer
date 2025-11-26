#include "VideoRenderer.h"
#include "VideoDecoder.h"
#include "libavutil/frame.h"
#include <QQuickFramebufferObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QDebug>
#include <QObject>
#include <QString>
#include <QFile>
#include <gl/gl.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

// Utility: load shader file
static QString loadShader(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to load shader:" << path;
        return QString();
    }
    return file.readAll();
}

class GLVideoRenderer : protected QOpenGLFunctions {
public:
    GLVideoRenderer();
    ~GLVideoRenderer();

    void initialize();
    void updateTextures(AVFrame* frame);
    void render();

private:
    bool initialized_;
    GLuint textureY_;
    GLuint textureU_;
    GLuint textureV_;
    GLuint shaderProgram_;
    GLuint vbo_;
};

GLVideoRenderer::GLVideoRenderer()
    : initialized_(false)
    , textureY_(0)
    , textureU_(0)
    , textureV_(0)
    , shaderProgram_(0)
    , vbo_(0)
{
}

GLVideoRenderer::~GLVideoRenderer() {
    if (textureY_) glDeleteTextures(1, &textureY_);
    if (textureU_) glDeleteTextures(1, &textureU_);
    if (textureV_) glDeleteTextures(1, &textureV_);
    if (shaderProgram_) glDeleteProgram(shaderProgram_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
}

void GLVideoRenderer::initialize() {
    if (initialized_) return;
    initializeOpenGLFunctions();

    QString vertexShaderSource = loadShader(":/shaders/vertex.vert");
    QString fragmentShaderSource = loadShader(":/shaders/fragment.frag");
    
    if (vertexShaderSource.isEmpty() || fragmentShaderSource.isEmpty()) {
        qCritical() << "Failed to load shader files";
        return;
    }
    
    // Compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    QByteArray vertexBytes = vertexShaderSource.toUtf8();
    const char* vertexData = vertexBytes.constData();
    glShaderSource(vertexShader, 1, &vertexData, nullptr);
    glCompileShader(vertexShader);
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    QByteArray fragmentBytes = fragmentShaderSource.toUtf8();
    const char* fragmentData = fragmentBytes.constData();
    glShaderSource(fragmentShader, 1, &fragmentData, nullptr);
    glCompileShader(fragmentShader);

    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vertexShader);
    glAttachShader(shaderProgram_, fragmentShader);
    glLinkProgram(shaderProgram_);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float vertices[] = {
        1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
       -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
       -1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
    };

    // Create a single VBO for the fullscreen quad
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glGenTextures(1, &textureY_);
    glGenTextures(1, &textureU_);
    glGenTextures(1, &textureV_);

    initialized_ = true;
}

void GLVideoRenderer::updateTextures(AVFrame* frame) {
    if (!frame || !initialized_) return;
    
    glBindTexture(GL_TEXTURE_2D, textureY_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame->width, frame->height, 0, GL_RED,
        GL_UNSIGNED_BYTE, frame->data[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glBindTexture(GL_TEXTURE_2D, textureU_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame->width / 2, frame->height / 2, 0, GL_RED,
        GL_UNSIGNED_BYTE, frame->data[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glBindTexture(GL_TEXTURE_2D, textureV_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame->width / 2, frame->height / 2, 0, GL_RED,
        GL_UNSIGNED_BYTE, frame->data[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   
}

void GLVideoRenderer::render() {
    if (!initialized_) return;
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(shaderProgram_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texcoord attribute (location = 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureY_);
    glUniform1i(glGetUniformLocation(shaderProgram_, "yTexture"), 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureU_);
    glUniform1i(glGetUniformLocation(shaderProgram_, "uTexture"), 1);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textureV_);
    glUniform1i(glGetUniformLocation(shaderProgram_, "vTexture"), 2);
    
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
} 

VideoRenderer::VideoRenderer(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
    , decoder_(nullptr)
    , glRenderer_(nullptr)
    , volume_(0.8)
    , muted_(false)
{
    setMirrorVertically(true); // Fix OpenGL vertical coordinate
    setTextureFollowsItemSize(true); // Make texture follow item size
    setFlag(QQuickItem::ItemHasContents, true); // Ensure item has renderable content
    decoder_ = new VideoDecoder(this);
    glRenderer_ = new GLVideoRenderer();
    
    connect(decoder_, &VideoDecoder::frameReady, this, [this](AVFrame* frame) {
        Q_UNUSED(frame);
        update(); // Request re-render
    });
    connect(decoder_, &VideoDecoder::errorOccurred, this, [](const QString& e){ qWarning() << e; });

    // Forward decoder state/duration/position to QML-facing signals
    connect(decoder_, &VideoDecoder::stateChanged, this, [this](VideoDecoder::PlaybackState s) {
        emit stateChanged(static_cast<int>(s));
        // When entering Playing state, kick the render loop once
        if (s == VideoDecoder::Playing) {
            update();
        }
    });
    connect(decoder_, &VideoDecoder::durationChanged, this, [this](qint64 d) {
        emit durationChanged(d);
    });
    connect(decoder_, &VideoDecoder::positionChanged, this, [this](qint64 p) {
        emit positionChanged(p);
    });
    connect(decoder_, &VideoDecoder::metadataChanged, this, [this]() {
        emit metadataChanged();
    });
    
    // Queue an initial update to start the render loop
    QMetaObject::invokeMethod(this, [this]() {
        update();
    }, Qt::QueuedConnection);
}

VideoRenderer::~VideoRenderer() {
    delete glRenderer_;
}

QString VideoRenderer::source() const {
    return source_;
}

void VideoRenderer::setSource(const QString& source) {
    if (source_ != source) {
        source_ = source;
        if (decoder_) {
            decoder_->loadFile(source);
        }
        emit sourceChanged();
        update();
    }
}

QQuickFramebufferObject::Renderer *VideoRenderer::createRenderer() const {
    return new VideoRendererInternal();
}

void VideoRenderer::renderToFbo(QOpenGLFramebufferObject* fbo) {
    Q_UNUSED(fbo);
    if (glRenderer_) {
        glRenderer_->render();
    }
}

int VideoRenderer::state() const {
    if (!decoder_)
        return 0;
    return static_cast<int>(decoder_->state());
}

qint64 VideoRenderer::duration() const {
    return decoder_ ? decoder_->duration() : 0;
}

qint64 VideoRenderer::position() const {
    return decoder_ ? decoder_->position() : 0;
}

qreal VideoRenderer::volume() const {
    return volume_;
}

void VideoRenderer::setVolume(qreal volume) {
    if (qFuzzyCompare(volume_, volume)) return;
    volume_ = qBound(0.0, volume, 1.0);
    emit volumeChanged(volume_);
    // TODO: Apply volume to audio output when audio is implemented
}

bool VideoRenderer::muted() const {
    return muted_;
}

void VideoRenderer::setMuted(bool muted) {
    if (muted_ == muted) return;
    muted_ = muted;
    emit mutedChanged(muted_);
    // TODO: Apply mute to audio output when audio is implemented
}

void VideoRenderer::play() {
    if (decoder_)
        decoder_->play();
}

void VideoRenderer::pause() {
    if (decoder_)
        decoder_->pause();
}

void VideoRenderer::stop() {
    if (decoder_)
        decoder_->stop();
}

void VideoRenderer::seek(qint64 position) {
    if (!decoder_)
        return;
    
    decoder_->seek(position);
    update();  // Request QML repaint
}

int VideoRenderer::videoWidth() const {
    return decoder_ ? decoder_->videoWidth() : 0;
}

int VideoRenderer::videoHeight() const {
    return decoder_ ? decoder_->videoHeight() : 0;
}

QString VideoRenderer::videoCodec() const {
    return decoder_ ? decoder_->videoCodec() : QString();
}

qint64 VideoRenderer::bitrate() const {
    return decoder_ ? decoder_->bitrate() : 0;
}

// ========== VideoRendererInternal implementation ==========

VideoRendererInternal::VideoRendererInternal()
    : item_(nullptr)
{
}

VideoRendererInternal::~VideoRendererInternal() {
}

void VideoRendererInternal::render() {
    if (!item_) return;
    item_->renderToFbo(framebufferObject());
}

void VideoRendererInternal::synchronize(QQuickFramebufferObject *item) {
    item_ = static_cast<VideoRenderer*>(item);
    
    if (item_->glRenderer_) {
        item_->glRenderer_->initialize();
    }
    
    VideoDecoder* decoder = item_->decoder_;
    if (decoder && decoder->hasVideo() && !decoder->isEof() && item_->glRenderer_) {
        AVFrame* frame = decoder->decodeFrame();
        if (frame) {
            item_->glRenderer_->updateTextures(frame);
        }
    }

    // Only request next frame while playing and not EOF
    if (item && decoder && !decoder->isEof() && decoder->state() == VideoDecoder::Playing) {
        item->update();
    }
}

QOpenGLFramebufferObject *VideoRendererInternal::createFramebufferObject(const QSize &size) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    return new QOpenGLFramebufferObject(size, format);
}
