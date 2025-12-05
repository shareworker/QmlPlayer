#include "VideoDecoder.h"
#include <QSize>
#include <QDateTime>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

VideoDecoder::VideoDecoder(QObject *parent)
    : QThread(parent)
{
    // Initialize FFmpeg (once globally)
    static bool ffmpeg_initialized = false;
    if (!ffmpeg_initialized) {
        avformat_network_init();
        ffmpeg_initialized = true;
    }
}

VideoDecoder::~VideoDecoder() {
    close();
}

void VideoDecoder::cleanup() {
    QMutexLocker locker(&mutex_);
    
    if (codec_context_) {
        avcodec_free_context(&codec_context_);
        codec_context_ = nullptr;
    }
    
    frame_queue_.clear();
    duration_ = 0;
    position_ = 0;
    
    videoWidth_ = 0;
    videoHeight_ = 0;
    videoCodec_.clear();
    bitrate_ = 0;
}

void VideoDecoder::setState(PlaybackState state) {
    if (state_ != state) {
        state_ = state;
        emit stateChanged(state_);
    }
}

bool VideoDecoder::open(AVFormatContext* formatCtx, int streamIndex) {
    if (!formatCtx || streamIndex < 0) {
        emit errorOccurred("Invalid format context or stream index");
        return false;
    }
    
    AVStream* stream = formatCtx->streams[streamIndex];
    time_base_ = stream->time_base;
    
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        emit errorOccurred("Video codec not found");
        return false;
    }
    
    codec_context_ = avcodec_alloc_context3(codec);
    if (!codec_context_) {
        emit errorOccurred("Failed to allocate video codec context");
        return false;
    }
    
    if (avcodec_parameters_to_context(codec_context_, stream->codecpar) < 0) {
        emit errorOccurred("Failed to copy video codec parameters");
        cleanup();
        return false;
    }
    
    if (avcodec_open2(codec_context_, codec, nullptr) < 0) {
        emit errorOccurred("Failed to open video codec");
        cleanup();
        return false;
    }
    
    // Extract metadata
    videoWidth_ = codec_context_->width;
    videoHeight_ = codec_context_->height;
    videoCodec_ = QString::fromUtf8(avcodec_get_name(codec_context_->codec_id));
    if (formatCtx->duration != AV_NOPTS_VALUE) {
        duration_ = formatCtx->duration * 1000 / AV_TIME_BASE;
    }
    bitrate_ = formatCtx->bit_rate;
    
    stop_requested_ = false;
    frame_queue_.start();
    
    emit metadataChanged();
    emit durationChanged(duration_);
    return true;
}

void VideoDecoder::close() {
    requestStop();
    if (isRunning()) {
        wait();
    }
    cleanup();
    setState(Stopped);
}

void VideoDecoder::setPacketQueue(ThreadSafeQueue<AVPacket*>* queue) {
    packet_queue_ = queue;
}

void VideoDecoder::flush() {
    flush_requested_ = true;
    frame_queue_.clear();
    if (codec_context_) {
        avcodec_flush_buffers(codec_context_);
    }
    // flush_requested_ will be reset by run() after it detects the flag
}

void VideoDecoder::requestStop() {
    stop_requested_ = true;
    frame_queue_.stop();
}

void VideoDecoder::run() {
    if (!packet_queue_ || !codec_context_) {
        emit errorOccurred("VideoDecoder not properly initialized");
        return;
    }
    
    AVFrame* decoded_frame = av_frame_alloc();
    if (!decoded_frame) {
        emit errorOccurred("Failed to allocate decoded frame");
        return;
    }
    
    qint64 start_time = -1;
    qint64 first_pts = -1;
    
    while (!stop_requested_) {
        if (flush_requested_) {
            start_time = -1;
            first_pts = -1;
            flush_requested_ = false;
            continue;
        }
        
        if (state_ != Playing) {
            start_time = -1;
            first_pts = -1;
            QThread::msleep(10);
            continue;
        }
        
        AVPacket* packet = nullptr;
        if (!packet_queue_->tryPop(packet)) {
            QThread::msleep(5);
            continue;
        }
        
        if (!packet) {
            continue;
        }
        
        int ret = avcodec_send_packet(codec_context_, packet);
        av_packet_free(&packet);
        
        if (ret < 0) {
            continue;
        }
        
        while (ret >= 0 && !stop_requested_) {
            ret = avcodec_receive_frame(codec_context_, decoded_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            if (ret < 0) {
                break;
            }
            
            qint64 pts_ms = 0;
            if (decoded_frame->pts != AV_NOPTS_VALUE) {
                pts_ms = av_rescale_q(decoded_frame->pts, time_base_, {1, 1000});
            }
            
            // Frame rate control
            qint64 now = QDateTime::currentMSecsSinceEpoch();
            if (start_time < 0) {
                start_time = now;
                first_pts = pts_ms;
            } else {
                qint64 elapsed = now - start_time;
                qint64 target = pts_ms - first_pts;
                if (target > elapsed) {
                    QThread::msleep(target - elapsed);
                }
            }
            
            AVFrame* output_frame = av_frame_clone(decoded_frame);
            if (output_frame) {
                if (position_ != pts_ms) {
                    position_ = pts_ms;
                    emit positionChanged(position_);
                }
                frame_queue_.push(output_frame);
                emit frameReady(output_frame);
            }
            av_frame_unref(decoded_frame);
        }
    }
    
    av_frame_free(&decoded_frame);
}

bool VideoDecoder::hasVideo() const {
    return codec_context_ != nullptr;
}

QSize VideoDecoder::videoSize() const {
    if (hasVideo()) {
        return QSize(codec_context_->width, codec_context_->height);
    }
    return QSize();
}

VideoDecoder::PlaybackState VideoDecoder::state() const {
    return state_;
}

qint64 VideoDecoder::duration() const {
    return duration_;
}

qint64 VideoDecoder::position() const {
    return position_;
}

int VideoDecoder::videoWidth() const {
    return videoWidth_;
}

int VideoDecoder::videoHeight() const {
    return videoHeight_;
}

QString VideoDecoder::videoCodec() const {
    return videoCodec_;
}

qint64 VideoDecoder::bitrate() const {
    return bitrate_;
}

void VideoDecoder::play() {
    if (state_ != Playing) {
        setState(Playing);
    }
}

void VideoDecoder::pause() {
    if (state_ == Playing) {
        setState(Paused);
    }
}

void VideoDecoder::stop() {
    if (state_ != Stopped) {
        setState(Stopped);
        // Seek reset handled by AVDemuxer
    }
}