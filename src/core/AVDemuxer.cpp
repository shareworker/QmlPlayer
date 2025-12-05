#include "AVDemuxer.h"
#include <QDebug>

extern "C" {
#include "libavcodec/packet.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/error.h"
}
#include <qobject.h>
#include <qstringview.h>
#include <qtypes.h>

AVDemuxer::AVDemuxer(QObject *parent)
    : QThread(parent) {}

AVDemuxer::~AVDemuxer() {
    close();
}

bool AVDemuxer::open(const QString& filename) {
    file_name_ = filename;
    QByteArray file_name_utf8 = filename.toUtf8();
    if (avformat_open_input(&format_context_, file_name_utf8.constData(), nullptr, nullptr) < 0) {
        emit errorOccurred("Failed to open file: " + filename);
        return false;
    }

    if (avformat_find_stream_info(format_context_, nullptr) < 0) {
        emit errorOccurred("Failed to find stream info");
        cleanup();
        return false;
    }

    for (unsigned int i = 0; i < format_context_->nb_streams; i++) {
        AVMediaType type = format_context_->streams[i]->codecpar->codec_type;
        if (type == AVMEDIA_TYPE_VIDEO && video_stream_index_ < 0) {
            video_stream_index_ = i;
        }
        else if (type == AVMEDIA_TYPE_AUDIO && audio_stream_index_ < 0) {
            audio_stream_index_ = i;
        }
    }

    if (video_stream_index_ < 0 && audio_stream_index_ < 0) {
        emit errorOccurred("No video or audio stream found");
        cleanup();
        return false;
    }
    
    emit opened();
    return true;
}

void AVDemuxer::close() {
    if (isRunning()) {
        stop_requested_ = true;
        video_queue_.stop();
        audio_queue_.stop();
        wait();
    }
    cleanup();
}

void AVDemuxer::cleanup() {
    clearQueues();
    if (format_context_) {
        avformat_close_input(&format_context_);
        format_context_ = nullptr;
    }

    video_stream_index_ = -1;
    audio_stream_index_ = -1;
    isEOF_ = false;
    stop_requested_ = false;
}

void AVDemuxer::clearQueues() {
    AVPacket* pkt = nullptr;
    while(video_queue_.tryPop(pkt)) {
        av_packet_free(&pkt);
    }
    while(audio_queue_.tryPop(pkt)) {
        av_packet_free(&pkt);
    }

    video_queue_.clear();
    audio_queue_.clear();
}

void AVDemuxer::seek(qint64 position) {
    seek_target_.store(position);
}

void AVDemuxer::run() {
    if (!format_context_) return;

    stop_requested_ = false;
    isEOF_ = false;
    video_queue_.start();
    audio_queue_.start();

    while (!stop_requested_) {
        qint64 seekMs = seek_target_.exchange(-1);
        if (seekMs >= 0) {
            int64_t seekTarget = seekMs * AV_TIME_BASE / 1000;
            if (av_seek_frame(format_context_, -1, seekTarget, AVSEEK_FLAG_BACKWARD) >= 0) {
                avformat_flush(format_context_);
                clearQueues();
                isEOF_ = false;
            }
        }

        AVPacket* packet = av_packet_alloc();
        if (!packet) {
            emit errorOccurred("Failed to allocate packet");
            break;
        }

        int ret = av_read_frame(format_context_, packet);
        if (ret < 0) {
            av_packet_free(&packet);
            if (ret == AVERROR_EOF) {
                isEOF_ = true;
                msleep(10);
                continue;
            } else {
                emit errorOccurred("Error reading frame");
                break;
            }
        }

        if (packet->stream_index == video_stream_index_) {
            video_queue_.push(packet);
        } else if (packet->stream_index == audio_stream_index_) {
            audio_queue_.push(packet);
        } else {
            av_packet_free(&packet);
        }
    }
}