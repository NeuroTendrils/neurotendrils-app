#pragma once

#include <QColor>
#include <QVector>
#include <QWidget>

// Multi-channel EEG strip chart for recent samples (downsampled for paint).
class EegStreamView : public QWidget {
    Q_OBJECT

public:
    explicit EegStreamView(QWidget* parent = nullptr);

    void setChannelCount(int channels);
    void setThemeColors(const QColor& background, const QColor& foreground, const QColor& muted,
                        const QColor& accent, const QColor& border);
    void clear();
    void appendSamples(const QVector<QVector<float>>& samples);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    static constexpr int kHistorySamples = 750; // ~3 s at 250 Hz
    static constexpr int kVisibleChannels = 8;

    int channelCount_ = 0;
    QVector<QVector<float>> history_; // [channel][time]
    QColor background_;
    QColor foreground_;
    QColor muted_;
    QColor accent_;
    QColor border_;
};
