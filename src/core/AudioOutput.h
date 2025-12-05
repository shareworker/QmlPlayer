#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <QObject>
#include <QThread>
#include <QAudioSink>
#include <QAudioFormat>
#include <QIODevice>
#include <atomic>
#include <qevent.h>
#include <qobject.h>
#include <qtmetamacros.h>

class AudioDecoder;
class AudioOutput : public QThread {
    Q_OBJECT
public:
    explicit AudioOutput(QObject* parent = nullptr);
    ~AudioOutput();
    void start(AudioDecoder* decoder);
    void stop();

    qreal volume() const;
    void setVolume(qreal volume);
    bool isMuted() const;
    void setMuted(bool muted);
    qint64 currentPts() const { return current_pts_.load(); }

    void pause();
    void resume();

signals:
    void volumeChanged(qreal volume);
    void mutedChanged(bool muted);
    void errorOccurred(const QString& message);

protected:
    void run() override;

private:
    void initAudioOutput();
    
    AudioDecoder* decoder_ = nullptr;
    QAudioSink* audio_sink_ = nullptr;
    QIODevice* audio_io_ = nullptr;
    
    qreal volume_ = 1.0;
    bool muted_ = false;
    std::atomic<bool> paused_{false};
    std::atomic<bool> stop_requested_{false};
    std::atomic<qint64> current_pts_{0};
    int sample_rate_ = 48000;
    int channels_ = 2;
};

#endif // AUDIOOUTPUT_H