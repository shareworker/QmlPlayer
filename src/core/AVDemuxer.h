#ifndef AVDEMUXER_H
#define AVDEMUXER_H

#include <QThread>
#include <QMutex>
#include <QString>
#include <atomic>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/packet.h>
}

#include "ThreadSafeQueue.h"

class AVDemuxer : public QThread {
    Q_OBJECT
public:
    explicit AVDemuxer(QObject *parent = nullptr);
    ~AVDemuxer();

    bool open(const QString& filename);
    void close();
    void seek(qint64 timestampMs);

    ThreadSafeQueue<AVPacket*>& videoQueue() { return video_queue_; }
    ThreadSafeQueue<AVPacket*>& audioQueue() { return audio_queue_; }

    int videoStreamIndex() const { return video_stream_index_; }
    int audioStreamIndex() const { return audio_stream_index_; }

    AVFormatContext* formatContext() const { return format_context_; }
    bool isEndOfFile() const { return isEOF_; }

signals:
    void errorOccurred(QString message);
    void opened();

protected:
    void run() override;

private:
    void cleanup();
    void clearQueues();

    QString file_name_;
    AVFormatContext* format_context_ = nullptr;
    int video_stream_index_ = -1;
    int audio_stream_index_ = -1;

    ThreadSafeQueue<AVPacket*> video_queue_{100};
    ThreadSafeQueue<AVPacket*> audio_queue_{200};

    std::atomic<bool> stop_requested_{false};
    std::atomic<bool> isEOF_{false};
    std::atomic<qint64> seek_target_{-1};
};

#endif // AVDEMUXER_H