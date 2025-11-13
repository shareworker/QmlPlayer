#include "VideoRenderer.h"
#include "VideoDecoder.h"
#include <QQuickFramebufferObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QDebug>
#include <QObject>
#include <QString>
#include <QFile>

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

VideoRenderer::VideoRenderer(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
    , decoder_(nullptr)
{
    setMirrorVertically(true); // Fix OpenGL vertical coordinate
    setTextureFollowsItemSize(true); // Make texture follow item size
    setFlag(QQuickItem::ItemHasContents, true); // Ensure item has renderable content
    decoder_ = new VideoDecoder(this);
    
    connect(decoder_, &VideoDecoder::frameReady, this, [this](AVFrame* frame) {
        update(); // Request re-render
    });
    connect(decoder_, &VideoDecoder::errorOccurred, this, [](const QString& e){ qWarning() << e; });
    
    // Queue an initial update to start the render loop
    QMetaObject::invokeMethod(this, [this]() {
        update();
    }, Qt::QueuedConnection);
}

VideoRenderer::~VideoRenderer() {
    // decoder_ will be deleted by Qt parent-child relationship
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

// ========== VideoRendererInternal implementation ==========

VideoRendererInternal::VideoRendererInternal()
    : textureY_(0)
    , textureU_(0)
    , textureV_(0)
    , shaderProgram_(0)
    , vertexArray_(0)
    , decoder_(nullptr)
{
    initializeOpenGLFunctions();
    initShaders();
}

VideoRendererInternal::~VideoRendererInternal() {
    if (textureY_) glDeleteTextures(1, &textureY_);
    if (textureU_) glDeleteTextures(1, &textureU_);
    if (textureV_) glDeleteTextures(1, &textureV_);
    if (shaderProgram_) glDeleteProgram(shaderProgram_);
    if (vertexArray_) glDeleteVertexArrays(1, &vertexArray_);
}

void VideoRendererInternal::initShaders() {
    // Load shaders from resources
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
    
    // Link program
    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vertexShader);
    glAttachShader(shaderProgram_, fragmentShader);
    glLinkProgram(shaderProgram_);
    
    // Delete shader objects (no longer needed after link)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Create VAO and VBO
    glGenVertexArrays(1, &vertexArray_);
    GLuint VBO;
    glGenBuffers(1, &VBO);
    
    // Vertex data (position + texcoord)
    float vertices[] = {
        // position    // texcoord
         1.0f,  1.0f, 0.0f,  1.0f, 0.0f,   // right-top
         1.0f, -1.0f, 0.0f,  1.0f, 1.0f,   // right-bottom
        -1.0f, -1.0f, 0.0f,  0.0f, 1.0f,   // left-bottom
        -1.0f,  1.0f, 0.0f,  0.0f, 0.0f    // left-top
    };
    
    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texcoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Create YUV textures
    glGenTextures(1, &textureY_);
    glGenTextures(1, &textureU_);
    glGenTextures(1, &textureV_);
}

void VideoRendererInternal::updateTextures(AVFrame* frame) {
    if (!frame) return;
    
    // Update Y texture
    glBindTexture(GL_TEXTURE_2D, textureY_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame->width, frame->height, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Update U texture
    glBindTexture(GL_TEXTURE_2D, textureU_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame->width/2, frame->height/2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Update V texture
    glBindTexture(GL_TEXTURE_2D, textureV_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame->width/2, frame->height/2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void VideoRendererInternal::render() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    if (!decoder_ || !decoder_->hasVideo()) {
        return;
    }
    
    glUseProgram(shaderProgram_);
    glBindVertexArray(vertexArray_);
    
    // Bind texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureY_);
    glUniform1i(glGetUniformLocation(shaderProgram_, "yTexture"), 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureU_);
    glUniform1i(glGetUniformLocation(shaderProgram_, "uTexture"), 1);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textureV_);
    glUniform1i(glGetUniformLocation(shaderProgram_, "vTexture"), 2);
    
    // Draw quad
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void VideoRendererInternal::synchronize(QQuickFramebufferObject *item) {
    VideoRenderer* renderer = static_cast<VideoRenderer*>(item);
    decoder_ = renderer->decoder_;
    
    // Initialize shader if needed
    if (!shaderProgram_) {
        initShaders();
    }
    
    // Fetch latest frame and update textures
    if (decoder_ && decoder_->hasVideo()) {
        AVFrame* frame = decoder_->decodeFrame();
        if (frame) {
            updateTextures(frame);
        }
    }

    // Request next frame
    if (item)
        item->update();
}

QOpenGLFramebufferObject *VideoRendererInternal::createFramebufferObject(const QSize &size) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    return new QOpenGLFramebufferObject(size, format);
}
