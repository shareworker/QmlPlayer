#include "AudioOutput.h"
#include "AudioDecoder.h"
#include <QAudioDevice>
#include <QMediaDevices>
#include <cstddef>
#include <cstdint>
#include <qaudioformat.h>
#include <qaudiosink.h>
#include <qmediadevices.h>
#include <qthread.h>
#include <QDebug>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/avutil.h>
}

AudioOutput::AudioOutput(QObject* parent) : QThread(parent) {
}

AudioOutput::~AudioOutput() {
    stop();
}

void AudioOutput::start(AudioDecoder* decoder) {
    if (isRunning()) stop();
    decoder_ = decoder;
    stop_requested_ = false;
    paused_ = false;
    QThread::start();
}

void AudioOutput::stop() {
    stop_requested_ = true;
    if (isRunning()) wait();
    if (audio_sink_) {
        audio_sink_->stop();
        delete audio_sink_;
        audio_sink_ = nullptr;
        audio_io_ = nullptr;
    }
}

void AudioOutput::pause() {
    paused_ = true;
    if (audio_sink_) {
        audio_sink_->suspend();
    }
}

void AudioOutput::resume() {
    paused_ = false;
    if (audio_sink_) {
        audio_sink_->resume();
    }
}

qreal AudioOutput::volume() const {
    return volume_;
}

void AudioOutput::setVolume(qreal volume) {
    volume_ = volume;
    if (audio_sink_) audio_sink_->setVolume(muted_ ? 0.0 : volume_);
    emit volumeChanged(volume_);
}

bool AudioOutput::isMuted() const {
    return muted_;
}

void AudioOutput::setMuted(bool muted) {
    muted_ = muted;
    if (audio_sink_) {
        audio_sink_->setVolume(muted_ ? 0.0 : volume_);
    }
    emit mutedChanged(muted_);
}

void AudioOutput::initAudioOutput() {
    QAudioFormat format;
    format.setSampleRate(sample_rate_);
    format.setChannelCount(channels_);
    format.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    
    if (!device.isFormatSupported(format)) {
        qWarning() << "Audio format not supported";
        emit errorOccurred("Audio format not supported");
        return;
    }

    audio_sink_ = new QAudioSink(device, format);
    audio_sink_->setVolume(muted_ ? 0.0 : volume_);
    audio_io_ = audio_sink_->start();
}

void AudioOutput::run() {
    if (!decoder_) {
        emit errorOccurred("No decoder");
        return;
    }

    initAudioOutput();
    if (!audio_io_) {
        return;
    }

    auto& frame_queue = decoder_->frameQueue();
    while (!stop_requested_) {
        if (paused_) {
            QThread::msleep(10);
            continue;
        }
        AVFrame* frame = nullptr;
        if (!frame_queue.pop(frame)) {
            break;
        }

        if (!frame) continue;
        if (frame->pts != AV_NOPTS_VALUE) {
            current_pts_.store(frame->pts);
        }

        int data_size = frame->nb_samples * channels_ * sizeof(int16_t);
        const char* data = reinterpret_cast<const char*>(frame->data[0]);
        int written = 0;
        while (written < data_size && !stop_requested_) {
            int bytes_free = audio_sink_->bytesFree();
            if (bytes_free > 0) {
                int to_write = qMin(bytes_free, data_size - written);
                int result = audio_io_->write(data + written, to_write);
                if (result > 0) {
                    written += result;
                } else {
                    QThread::msleep(5);
                }
            } else {
                QThread::msleep(5);
            }
        }
        av_frame_free(&frame);
    }

    if (audio_sink_) {
        audio_sink_->stop();
    }
}