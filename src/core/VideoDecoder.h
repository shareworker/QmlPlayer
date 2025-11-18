#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QObject>
#include <QString>
#include <QMutex>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

class VideoDecoder : public QObject
{
    Q_OBJECT
public:
    explicit VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder();

    bool loadFile(const QString& filename);
    bool hasVideo() const;
    QSize videoSize() const;
    AVFrame* decodeFrame();
    void seekTo(qint64 position);
    bool isEof() const;

signals:
    void frameReady(AVFrame* frame);
    void errorOccurred(const QString& error);

private:
    void cleanup();

    AVFormatContext* format_context_;
    AVCodecContext* codec_context_;
    SwsContext* sws_context_;
    int video_stream_index_;
    AVFrame* frame_;
    AVFrame* rgb_frame_;
    uint8_t* rgb_buffer_;
    bool eof_;
    mutable QMutex mutex_;  // Protect cross-thread access
};

#endif // VIDEODECODER_H
