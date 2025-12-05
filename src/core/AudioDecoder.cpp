#include "AudioDecoder.h"
#include <QDebug>

extern "C" {
#include "libavcodec/packet.h"
#include "libavutil/channel_layout.h"
#include "libavutil/frame.h"
#include "libswresample/swresample.h"
}

#include <qthread.h>
#include <qtmetamacros.h>

AudioDecoder::AudioDecoder(QObject *parent) : QThread(parent) {
    
}

AudioDecoder::~AudioDecoder() {
    close();
}

bool AudioDecoder::open(AVFormatContext *formatContext, int streamIndex) {
    if (!formatContext || streamIndex < 0) {
        emit errorOccurred("Invalid format context or stream index");
        return false;
    }

    AVStream* stream = formatContext->streams[streamIndex];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        emit errorOccurred("Unsupported codec");
        return false;
    }
    
    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        emit errorOccurred("Failed to allocate codec context");
        return false;
    }
    
    if (avcodec_parameters_to_context(codec_ctx_, stream->codecpar) < 0) {
        emit errorOccurred("Failed to copy codec parameters to context");
        cleanup();
        return false;
    }
    
    if (avcodec_open2(codec_ctx_, codec, nullptr) < 0) {
        emit errorOccurred("Failed to open codec");
        cleanup();
        return false;
    }
    
    if (!initResampler()) {
        cleanup();
        return false;
    }

    stop_requested_ = false;
    frame_queue_.start();
    return true;
}

bool AudioDecoder::initResampler() {
    swr_ctx_ = swr_alloc();
    if (!swr_ctx_) {
        emit errorOccurred("Failed to allocate SwrContext");
        return false;
    }

    AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_STEREO;
    AVChannelLayout in_ch_layout = codec_ctx_->ch_layout;

    av_opt_set_chlayout(swr_ctx_, "in_chlayout", &in_ch_layout, 0);
    av_opt_set_chlayout(swr_ctx_, "out_chlayout", &out_ch_layout, 0);
    av_opt_set_int(swr_ctx_, "in_sample_rate", codec_ctx_->sample_rate, 0);
    av_opt_set_int(swr_ctx_, "out_sample_rate", out_sample_rate_, 0);
    av_opt_set_sample_fmt(swr_ctx_, "in_sample_fmt", codec_ctx_->sample_fmt, 0);
    av_opt_set_sample_fmt(swr_ctx_, "out_sample_fmt", out_sample_fmt_, 0);
    
    if (swr_init(swr_ctx_) < 0) {
        emit errorOccurred("Failed to initialize SwrContext");
        swr_free(&swr_ctx_);
        return false; 
    }

    return true;
}

void AudioDecoder::close() {
    requestStop();
    if (isRunning()) wait();
    cleanup();
}

void AudioDecoder::cleanup() {
    if (swr_ctx_) {
        swr_free(&swr_ctx_);
        swr_ctx_ = nullptr;
    }

    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
        codec_ctx_ = nullptr;
    }
    frame_queue_.clear();
}

void AudioDecoder::setPacketQueue(ThreadSafeQueue<AVPacket*>* queue)
{
    packet_queue_ = queue;
}

void AudioDecoder::flush()
{
    flush_requested_ = true;
    frame_queue_.clear();
    if (codec_ctx_) {
        avcodec_flush_buffers(codec_ctx_);
    }
    // flush_requested_ will be reset by run()
}

void AudioDecoder::requestStop()
{
    stop_requested_ = true;
    frame_queue_.stop();
}

void AudioDecoder::run()
{
    if (!packet_queue_ || !codec_ctx_) {
        emit errorOccurred("AudioDecoder not properly initialized");
        return;
    }

    AVFrame* decoded_frame = av_frame_alloc();
    if (!decoded_frame) {
        emit errorOccurred("Failed to allocate decoded frame");
        return;
    }

    while (!stop_requested_) {
        if (flush_requested_) {
            flush_requested_ = false;
            QThread::msleep(10);
            continue;
        }

        AVPacket* packet = nullptr;
        if (!packet_queue_->pop(packet)) {
            break;
        }

        if (!packet) {
            continue;
        }

        int ret = avcodec_send_packet(codec_ctx_, packet);
        av_packet_free(&packet);

        if (ret < 0) {
            qWarning() << "Error sending audio packet for decoding";
            continue; 
        }

        while (ret >= 0 && !stop_requested_) {
            ret = avcodec_receive_frame(codec_ctx_, decoded_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            if (ret < 0) {
                qWarning() << "Error receiving audio frame";
                break;
            }

            AVFrame* resampled_frame = av_frame_alloc();
            resampled_frame->sample_rate = out_sample_rate_;
            resampled_frame->format = out_sample_fmt_;
            AVChannelLayout stereo_layout = AV_CHANNEL_LAYOUT_STEREO;
            av_channel_layout_copy(&resampled_frame->ch_layout, &stereo_layout);
            resampled_frame->nb_samples = swr_get_out_samples(swr_ctx_, decoded_frame->nb_samples);
            if (av_frame_get_buffer(resampled_frame, 0) < 0) {
                av_frame_free(&resampled_frame);
                continue;
            }

            int converted = swr_convert(swr_ctx_,
                                        resampled_frame->data, resampled_frame->nb_samples,
                                        (const uint8_t**)decoded_frame->data, decoded_frame->nb_samples);

            if (converted < 0) {
                av_frame_free(&resampled_frame);
                continue;
            }

            resampled_frame->nb_samples = converted;
            resampled_frame->pts = decoded_frame->pts;

            frame_queue_.push(resampled_frame);
            av_frame_unref(decoded_frame);
        }
    }
    av_frame_free(&decoded_frame);
}