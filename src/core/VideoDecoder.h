#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QObject>
#include <QString>
#include <QMutex>
#include <atomic>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

class VideoDecoder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(PlaybackState state READ state NOTIFY stateChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
public:
    enum PlaybackState {
        Stopped,
        Playing,
        Paused
    };
    Q_ENUM(PlaybackState)
    explicit VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder();

    bool loadFile(const QString& filename);
    bool hasVideo() const;
    QSize videoSize() const;
    AVFrame* decodeFrame();
    bool isEof() const;

    PlaybackState state() const;
    qint64 duration() const;
    qint64 position() const;

signals:
    void stateChanged(PlaybackState state);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);
    void frameReady(AVFrame* frame);
    void errorOccurred(const QString& error);

public slots:
    void play();
    void pause();
    void stop();
    void seek(qint64 position); 

private:
    void cleanup();
    void setState(PlaybackState state);

    AVFormatContext* format_context_;
    AVCodecContext* codec_context_;
    AVCodecParameters* codec_params_;
    struct SwsContext* sws_context_;
    int video_stream_index_;
    AVFrame* frame_;
    AVFrame* rgb_frame_;
    uint8_t* rgb_buffer_;
    bool eof_;
    mutable QMutex mutex_;  // Protect cross-thread access

    PlaybackState state_;
    qint64 duration_;
    qint64 position_;
    std::atomic<qint64> pending_seek_ms_;
};

#endif // VIDEODECODER_H
