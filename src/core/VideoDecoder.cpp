#include "VideoDecoder.h"
#include <QObject>
#include <QString>
#include <QSize>
#include <QFile>
#include <QUrl>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

VideoDecoder::VideoDecoder(QObject *parent)
    : QObject(parent)
    , format_context_(nullptr)
    , codec_context_(nullptr)
    , sws_context_(nullptr)
    , video_stream_index_(-1)
    , frame_(nullptr)
    , rgb_frame_(nullptr)
    , rgb_buffer_(nullptr)
    , eof_(false)
    , state_(Stopped)
    , duration_(0)
    , position_(0)
{
    pending_seek_ms_.store(-1, std::memory_order_relaxed);
    // Initialize FFmpeg (once globally)
    static bool ffmpeg_initialized = false;
    if (!ffmpeg_initialized) {
        // av_register_all() is deprecated in modern FFmpeg; not needed
        avformat_network_init();
        ffmpeg_initialized = true;
    }
    
    // Allocate frames
    frame_ = av_frame_alloc();
    rgb_frame_ = av_frame_alloc();
}

VideoDecoder::~VideoDecoder() {
    cleanup();
}

void VideoDecoder::cleanup() {
    QMutexLocker locker(&mutex_);
    
    if (format_context_) {
        avformat_close_input(&format_context_);
        format_context_ = nullptr;
    }
    
    if (codec_context_) {
        avcodec_free_context(&codec_context_);
        codec_context_ = nullptr;
    }
    
    if (sws_context_) {
        sws_freeContext(sws_context_);
        sws_context_ = nullptr;
    }
    
    if (rgb_buffer_) {
        av_free(rgb_buffer_);
        rgb_buffer_ = nullptr;
    }
    
    if (frame_) {
        av_frame_free(&frame_);
        frame_ = nullptr;
    }
    
    if (rgb_frame_) {
        av_frame_free(&rgb_frame_);
        rgb_frame_ = nullptr;
    }
    
    video_stream_index_ = -1;
    eof_ = false;
    duration_ = 0;
    position_ = 0;
    pending_seek_ms_.store(-1, std::memory_order_relaxed);
    setState(Stopped);
}

void VideoDecoder::setState(PlaybackState state) {
    if (state_ != state) {
        state_ = state;
        emit stateChanged(state_);
    }
}

bool VideoDecoder::loadFile(const QString& filename) {
    QString path = filename;
    
    // For file:/// URL, convert to local path (FFmpeg on Windows does not accept file:/// for local files)
    if (path.startsWith(QLatin1String("file:///"), Qt::CaseInsensitive)) {
        QUrl u(path);
        path = u.toLocalFile();
    } else if (path.startsWith(QLatin1String("file:"), Qt::CaseInsensitive)) {
        QUrl u(path);
        path = u.toLocalFile();
    }
    // Other URLs (http:, rtsp:, etc.) remain unchanged
    
    QByteArray utf8 = path.toUtf8();
    const char* cstr = utf8.constData();
    
    int err = avformat_open_input(&format_context_, cstr, nullptr, nullptr);
    if (err != 0) {
        char buf[256];
        av_strerror(err, buf, sizeof(buf));
        emit errorOccurred(QString::fromLatin1("Cannot open file: %1").arg(QString::fromLatin1(buf)));
        return false;
    }

    if ((err = avformat_find_stream_info(format_context_, nullptr)) < 0) {
        char buf[256];
        av_strerror(err, buf, sizeof(buf));
        emit errorOccurred(QString::fromLatin1("Cannot find stream information: %1").arg(QString::fromLatin1(buf)));
        return false;
    }

    for (unsigned int i = 0; i < format_context_->nb_streams; i++) {
        if (format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index_ = i;
            break;
        }
    }
    if (video_stream_index_ < 0) {
        emit errorOccurred("No video stream found");
        return false;
    }

    const AVCodec *codec = avcodec_find_decoder(format_context_->streams[video_stream_index_]->codecpar->codec_id);
    codec_context_ = avcodec_alloc_context3(const_cast<AVCodec*>(codec));
    avcodec_parameters_to_context(codec_context_, format_context_->streams[video_stream_index_]->codecpar);
    if ((err = avcodec_open2(codec_context_, const_cast<AVCodec*>(codec), nullptr)) < 0) {
        char buf[256];
        av_strerror(err, buf, sizeof(buf));
        emit errorOccurred(QString::fromLatin1("Cannot open decoder: %1").arg(QString::fromLatin1(buf)));
        return false;
    }

    // Compute duration in milliseconds if available
    if (format_context_->duration != AV_NOPTS_VALUE) {
        duration_ = format_context_->duration * 1000 / AV_TIME_BASE;
        emit durationChanged(duration_);
    } else {
        duration_ = 0;
        emit durationChanged(duration_);
    }

    eof_ = false;  // Reset EOF on new file
    setState(Playing);
    return true;
}

AVFrame* VideoDecoder::decodeFrame() {
    QMutexLocker locker(&mutex_);
    
    if (!format_context_ || !codec_context_) {
        return nullptr;
    }
    
    // Apply any pending seek request (from UI thread) on this decoder thread
    qint64 pending = pending_seek_ms_.exchange(-1, std::memory_order_relaxed);
    if (pending >= 0) {
        int64_t target_pts = pending * AV_TIME_BASE / 1000; // ms to AV_TIME_BASE
        if (av_seek_frame(format_context_, -1, target_pts, AVSEEK_FLAG_BACKWARD) < 0) {
            emit errorOccurred("Seek failed");
        } else {
            avcodec_flush_buffers(codec_context_);
            eof_ = false;
            position_ = pending;
            emit positionChanged(position_);
        }
    }

    if (eof_) {
        return nullptr;
    }

    // Only decode while playing
    if (state_ != Playing) {
        return nullptr;
    }
    
    AVPacket *packet = av_packet_alloc();
    while (av_read_frame(format_context_, packet) >= 0) {
        if (packet->stream_index == video_stream_index_) {
            avcodec_send_packet(codec_context_, packet);

            if (avcodec_receive_frame(codec_context_, frame_) == 0) {
                if (frame_->pts != AV_NOPTS_VALUE) {
                    AVRational tb = format_context_->streams[video_stream_index_]->time_base;
                    qint64 pts_ms = frame_->pts * 1000 * tb.num / tb.den;
                    if (position_ != pts_ms) {
                        position_ = pts_ms;
                        emit positionChanged(position_);
                    }
                }
                av_packet_unref(packet);
                av_packet_free(&packet);  // Fix memory leak
                return frame_;
            }
        }
        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    eof_ = true;  // Mark EOF when no more frames
    setState(Stopped);
    return nullptr;
}

bool VideoDecoder::hasVideo() const {
    return video_stream_index_ != -1 && codec_context_ != nullptr;
}

bool VideoDecoder::isEof() const {
    return eof_;
}

QSize VideoDecoder::videoSize() const {
    if (hasVideo()) {
        return QSize(codec_context_->width, codec_context_->height);
    }
    return QSize();
}

void VideoDecoder::seek(qint64 position) {
    // Record pending seek in milliseconds; actual FFmpeg seek will be
    // performed on the decoder/render thread inside decodeFrame().
    pending_seek_ms_.store(position, std::memory_order_relaxed);
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
        seek(0);
    }
}