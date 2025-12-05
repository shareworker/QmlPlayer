#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QThread>
#include <QString>
#include <QMutex>
#include <atomic>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include "ThreadSafeQueue.h"

class VideoDecoder : public QThread
{
    Q_OBJECT
    Q_PROPERTY(PlaybackState state READ state NOTIFY stateChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(int videoWidth READ videoWidth NOTIFY metadataChanged)
    Q_PROPERTY(int videoHeight READ videoHeight NOTIFY metadataChanged)
    Q_PROPERTY(QString videoCodec READ videoCodec NOTIFY metadataChanged)
    Q_PROPERTY(qint64 bitrate READ bitrate NOTIFY metadataChanged)
public:
    enum PlaybackState {
        Stopped,
        Playing,
        Paused
    };
    Q_ENUM(PlaybackState)
    explicit VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder();

    bool open(AVFormatContext* formatCtx, int streamIndex);
    void close();
    
    void setPacketQueue(ThreadSafeQueue<AVPacket*>* queue);
    
    ThreadSafeQueue<AVFrame*>& frameQueue() { return frame_queue_; }
    
    bool hasVideo() const;
    QSize videoSize() const;
    
    void flush();
    void requestStop();

    PlaybackState state() const;
    qint64 duration() const;
    qint64 position() const;
    int videoWidth() const;
    int videoHeight() const;
    QString videoCodec() const;
    qint64 bitrate() const;

signals:
    void stateChanged(PlaybackState state);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);
    void metadataChanged();
    void frameReady(AVFrame* frame);
    void errorOccurred(const QString& error);

protected:
    void run() override;

public slots:
    void play();
    void pause();
    void stop();

private:
    void cleanup();
    void setState(PlaybackState state);

    AVCodecContext* codec_context_ = nullptr;
    ThreadSafeQueue<AVPacket*>* packet_queue_ = nullptr;
    ThreadSafeQueue<AVFrame*> frame_queue_{30};
    AVRational time_base_{};
    std::atomic<bool> stop_requested_{false};
    std::atomic<bool> flush_requested_{false};
    mutable QMutex mutex_;

    PlaybackState state_ = Stopped;
    qint64 duration_ = 0;
    qint64 position_ = 0;
    
    // Metadata
    int videoWidth_ = 0;
    int videoHeight_ = 0;
    QString videoCodec_;
    qint64 bitrate_ = 0;
};

#endif // VIDEODECODER_H
