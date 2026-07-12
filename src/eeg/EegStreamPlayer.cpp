#include "eeg/EegStreamPlayer.hpp"

#include "inference/onnx_runner.hpp"
#include "io/bundle_reader.hpp"
#include "preprocess/preprocessor.hpp"
#include "preprocess/window_buffer.hpp"

#include <QDateTime>
#include <QTimer>

#include <filesystem>
#include <stdexcept>
#include <utility>
#include <vector>

struct EegStreamPlayer::Impl {
    eeg::ModelMetadata metadata;
    eeg::StreamManifest manifest;
    std::vector<eeg::Sample> samples;
    std::unique_ptr<eeg::OnnxRunner> runner;
    std::unique_ptr<eeg::Preprocessor> preprocessor;
    std::unique_ptr<eeg::WindowBuffer> buffer;
};

EegStreamPlayer::EegStreamPlayer(QObject* parent)
    : QObject(parent)
    , impl_(std::make_unique<Impl>()) {
    timer_ = new QTimer(this);
    timer_->setInterval(16);
    connect(timer_, &QTimer::timeout, this, &EegStreamPlayer::onTick);
    setStatus(QStringLiteral("No bundle loaded"));
}

EegStreamPlayer::~EegStreamPlayer() = default;

int EegStreamPlayer::channelCount() const {
    return loaded_ ? impl_->metadata.n_chans : 0;
}

float EegStreamPlayer::sampleRateHz() const {
    return loaded_ ? impl_->metadata.sfreq : 0.0f;
}

qint64 EegStreamPlayer::sampleCount() const {
    return loaded_ ? static_cast<qint64>(impl_->samples.size()) : 0;
}

QStringList EegStreamPlayer::classNames() const {
    QStringList names;
    if (!loaded_) {
        return names;
    }
    for (const std::string& name : impl_->metadata.class_names) {
        names.append(QString::fromStdString(name));
    }
    return names;
}

bool EegStreamPlayer::load(const QString& bundleDir, QString* error) {
    stop();
    loaded_ = false;
    impl_->runner.reset();
    impl_->preprocessor.reset();
    impl_->buffer.reset();
    impl_->samples.clear();

    try {
        const std::filesystem::path dir = bundleDir.toStdString();
        impl_->metadata = eeg::BundleReader::load_metadata(dir);
        if (!impl_->metadata.onnx.external_data_file.empty()) {
            eeg::BundleReader::external_data_path(dir);
        }
        const auto modelPath = eeg::BundleReader::model_path(dir);
        impl_->runner = std::make_unique<eeg::OnnxRunner>(modelPath, impl_->metadata);
        impl_->preprocessor = std::make_unique<eeg::Preprocessor>(impl_->metadata);
        impl_->buffer = std::make_unique<eeg::WindowBuffer>(
            impl_->metadata.n_chans, impl_->metadata.n_times);
        impl_->manifest = eeg::BundleReader::load_stream_manifest(dir);
        impl_->samples = eeg::BundleReader::load_stream_samples(dir);
    } catch (const std::exception& ex) {
        if (error != nullptr) {
            *error = QString::fromUtf8(ex.what());
        }
        setStatus(QStringLiteral("Load failed"));
        return false;
    }

    bundlePath_ = bundleDir;
    loaded_ = true;
    resetPlayback();
    setStatus(QStringLiteral("Ready: %1 samples @ %2 Hz")
                  .arg(sampleCount())
                  .arg(sampleRateHz(), 0, 'f', 0));
    return true;
}

void EegStreamPlayer::play() {
    if (!loaded_ || impl_->samples.empty()) {
        return;
    }
    if (sampleIndex_ >= static_cast<qint64>(impl_->samples.size())) {
        resetPlayback();
    }
    playing_ = true;
    wallOriginMs_ = QDateTime::currentMSecsSinceEpoch();
    sampleOrigin_ = sampleIndex_;
    timer_->start();
    setStatus(QStringLiteral("Playing"));
}

void EegStreamPlayer::pause() {
    if (!playing_) {
        return;
    }
    playing_ = false;
    timer_->stop();
    setStatus(QStringLiteral("Paused"));
}

void EegStreamPlayer::stop() {
    playing_ = false;
    timer_->stop();
    resetPlayback();
    if (loaded_) {
        setStatus(QStringLiteral("Stopped"));
    }
}

void EegStreamPlayer::setPlaybackRate(double rate) {
    if (rate < 0.1) {
        rate = 0.1;
    } else if (rate > 8.0) {
        rate = 8.0;
    }
    const bool wasPlaying = playing_;
    if (wasPlaying) {
        // Re-anchor so catch-up stays continuous at the new rate.
        wallOriginMs_ = QDateTime::currentMSecsSinceEpoch();
        sampleOrigin_ = sampleIndex_;
    }
    playbackRate_ = rate;
}

void EegStreamPlayer::resetPlayback() {
    sampleIndex_ = 0;
    sampleOrigin_ = 0;
    wallOriginMs_ = 0;
    if (impl_->buffer) {
        impl_->buffer->clear();
    }
}

void EegStreamPlayer::setStatus(const QString& status) {
    statusText_ = status;
    emit statusChanged(status);
}

void EegStreamPlayer::onTick() {
    if (!playing_) {
        return;
    }
    processCatchUp();
}

void EegStreamPlayer::processCatchUp() {
    const double hz = static_cast<double>(impl_->metadata.sfreq);
    if (hz <= 0.0) {
        return;
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const double elapsedSec = static_cast<double>(nowMs - wallOriginMs_) / 1000.0;
    const qint64 targetIndex =
        sampleOrigin_ + static_cast<qint64>(elapsedSec * hz * playbackRate_);

    const qint64 total = static_cast<qint64>(impl_->samples.size());
    if (sampleIndex_ >= total) {
        playing_ = false;
        timer_->stop();
        setStatus(QStringLiteral("Finished"));
        emit finished();
        return;
    }

    const qint64 endExclusive = std::min(targetIndex + 1, total);
    if (endExclusive <= sampleIndex_) {
        return;
    }

    // Cap work per tick so the UI stays responsive if we fall behind.
    constexpr qint64 kMaxBatch = 500;
    const qint64 batchEnd = std::min(endExclusive, sampleIndex_ + kMaxBatch);

    QVector<QVector<float>> batch;
    batch.reserve(static_cast<int>(batchEnd - sampleIndex_));
    const qint64 startIndex = sampleIndex_;

    for (; sampleIndex_ < batchEnd; ++sampleIndex_) {
        const eeg::Sample& sample = impl_->samples[static_cast<std::size_t>(sampleIndex_)];
        QVector<float> channels;
        channels.reserve(static_cast<int>(sample.channels.size()));
        for (float value : sample.channels) {
            channels.append(value);
        }
        batch.append(channels);

        impl_->buffer->push(sample.channels);
        if (!impl_->buffer->full()) {
            continue;
        }

        const std::vector<float> prepared = impl_->preprocessor->apply(impl_->buffer->as_nct());
        const eeg::Prediction pred = impl_->runner->predict(prepared);

        EegDecodeResult result;
        result.classIndex = pred.class_index;
        result.className = QString::fromStdString(pred.class_name);
        result.probabilities.reserve(static_cast<int>(pred.probabilities.size()));
        for (float p : pred.probabilities) {
            result.probabilities.append(p);
        }
        if (result.classIndex >= 0
            && result.classIndex < result.probabilities.size()) {
            result.confidence = result.probabilities.at(result.classIndex);
        }
        result.trialIndex = sample.trial_index;
        result.sampleIndex = sampleIndex_;
        result.expectedLabel = -1;
        if (sample.trial_index >= 0 && sample.trial_index < impl_->manifest.n_trials) {
            result.expectedLabel =
                impl_->manifest.labels.at(static_cast<std::size_t>(sample.trial_index));
            if (result.expectedLabel >= 0
                && static_cast<std::size_t>(result.expectedLabel)
                    < impl_->metadata.class_names.size()) {
                result.expectedClassName = QString::fromStdString(
                    impl_->metadata.class_names.at(
                        static_cast<std::size_t>(result.expectedLabel)));
            }
        }

        emit prediction(result);
        impl_->buffer->clear();
    }

    if (!batch.isEmpty()) {
        emit samplesBatch(batch, startIndex);
    }

    if (sampleIndex_ >= total) {
        playing_ = false;
        timer_->stop();
        setStatus(QStringLiteral("Finished"));
        emit finished();
    }
}
