#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include <QThread>
#include <QMutex>
#include <atomic>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
}

#include "ThreadSafeQueue.h"

class AudioDecoder : public QThread {
    Q_OBJECT
public:
    explicit AudioDecoder(QObject *parent = nullptr);
    ~AudioDecoder();

    bool open(AVFormatContext *formatContext, int streamIndex);
    void close();

    void setPacketQueue(ThreadSafeQueue<AVPacket*>* queue);
    ThreadSafeQueue<AVFrame*>& frameQueue() { return frame_queue_; }

    int sampleRate() const { return out_sample_rate_; }
    int channels() const { return out_channels_; }
    AVSampleFormat sampleFormat() const { return out_sample_fmt_; }

    void flush();
    void requestStop();

signals:
    void errorOccurred(const QString& message);

protected:
    void run() override;

private:
    void cleanup();
    bool initResampler();

    AVCodecContext* codec_ctx_ = nullptr;
    SwrContext* swr_ctx_ = nullptr;
    ThreadSafeQueue<AVPacket*>* packet_queue_ = nullptr;
    ThreadSafeQueue<AVFrame*> frame_queue_{50};
    int out_sample_rate_ = 48000;
    int out_channels_ = 2;
    AVSampleFormat out_sample_fmt_ = AV_SAMPLE_FMT_S16;
    std::atomic<bool> stop_requested_{false};
    std::atomic<bool> flush_requested_{false};
};

#endif