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
{
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
    return true;
}

AVFrame* VideoDecoder::decodeFrame() {
    if (!format_context_ || !codec_context_) {
        return nullptr;
    }
    
    AVPacket *packet = av_packet_alloc();
    while (av_read_frame(format_context_, packet) >= 0) {
        if (packet->stream_index == video_stream_index_) {
            avcodec_send_packet(codec_context_, packet);

            if (avcodec_receive_frame(codec_context_, frame_) == 0) {
                av_packet_unref(packet);
                return frame_;
            }
        }
        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    return nullptr;
}

bool VideoDecoder::hasVideo() const {
    return video_stream_index_ != -1 && codec_context_ != nullptr;
}

QSize VideoDecoder::videoSize() const {
    if (hasVideo()) {
        return QSize(codec_context_->width, codec_context_->height);
    }
    return QSize();
}

void VideoDecoder::seekTo(qint64 position) {
    if (!hasVideo()) {
        return;
    }

    int64_t target_pts = position * AV_TIME_BASE / 1000; // ms to AV_TIME_BASE
    if (av_seek_frame(format_context_, -1, target_pts, AVSEEK_FLAG_BACKWARD) < 0) {
        emit errorOccurred("Seek failed");
        return;
    }

    // Flush decoder buffers
    avcodec_flush_buffers(codec_context_);
}