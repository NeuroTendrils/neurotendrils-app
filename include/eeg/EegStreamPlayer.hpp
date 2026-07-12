#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include <memory>

class QTimer;

struct EegDecodeResult {
    QString className;
    int classIndex = -1;
    float confidence = 0.0f;
    QVector<float> probabilities;
    int expectedLabel = -1;
    QString expectedClassName;
    int trialIndex = -1;
    qint64 sampleIndex = -1;
};

// Loads a v2 EEG bundle and replays test_stream.bin at ~250 Hz with the
// same preprocess + ONNX Runtime path as the in-tree eeg/ pipeline.
class EegStreamPlayer : public QObject {
    Q_OBJECT

public:
    explicit EegStreamPlayer(QObject* parent = nullptr);
    ~EegStreamPlayer() override;

    bool isLoaded() const { return loaded_; }
    bool isPlaying() const { return playing_; }
    QString bundlePath() const { return bundlePath_; }
    QString statusText() const { return statusText_; }

    int channelCount() const;
    float sampleRateHz() const;
    qint64 sampleCount() const;
    qint64 currentSampleIndex() const { return sampleIndex_; }
    QStringList classNames() const;

public slots:
    // Loads metadata, model, and stream. Returns false on failure (see error).
    bool load(const QString& bundleDir, QString* error = nullptr);
    void play();
    void pause();
    void stop();
    void setPlaybackRate(double rate);

signals:
    void statusChanged(const QString& status);
    // Batch of multichannel samples for plotting (channel-major per sample).
    void samplesBatch(const QVector<QVector<float>>& samples, qint64 startIndex);
    void prediction(const EegDecodeResult& result);
    void finished();

private slots:
    void onTick();

private:
    void setStatus(const QString& status);
    void resetPlayback();
    void processCatchUp();

    struct Impl;
    std::unique_ptr<Impl> impl_;

    QTimer* timer_ = nullptr;
    QString bundlePath_;
    QString statusText_;
    bool loaded_ = false;
    bool playing_ = false;
    double playbackRate_ = 1.0;
    qint64 sampleIndex_ = 0;
    qint64 wallOriginMs_ = 0;
    qint64 sampleOrigin_ = 0;
};
