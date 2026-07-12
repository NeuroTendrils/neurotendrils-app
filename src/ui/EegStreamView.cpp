#include "ui/EegStreamView.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QSizePolicy>
#include <algorithm>

EegStreamView::EegStreamView(QWidget* parent)
    : QWidget(parent) {
    setMinimumHeight(220);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    background_ = QColor(QStringLiteral("#0e1220"));
    foreground_ = QColor(QStringLiteral("#e7eaf2"));
    muted_ = QColor(QStringLiteral("#97a0b5"));
    accent_ = QColor(QStringLiteral("#6f96ff"));
    border_ = QColor(QStringLiteral("#2a3148"));
}

void EegStreamView::setChannelCount(int channels) {
    channelCount_ = std::max(0, channels);
    history_.clear();
    history_.resize(channelCount_);
    for (auto& row : history_) {
        row.reserve(kHistorySamples);
    }
    update();
}

void EegStreamView::setThemeColors(const QColor& background, const QColor& foreground,
                                   const QColor& muted, const QColor& accent,
                                   const QColor& border) {
    background_ = background;
    foreground_ = foreground;
    muted_ = muted;
    accent_ = accent;
    border_ = border;
    update();
}

void EegStreamView::clear() {
    for (auto& row : history_) {
        row.clear();
    }
    update();
}

void EegStreamView::appendSamples(const QVector<QVector<float>>& samples) {
    if (channelCount_ <= 0 || samples.isEmpty()) {
        return;
    }

    for (const QVector<float>& sample : samples) {
        const int n = std::min(channelCount_, static_cast<int>(sample.size()));
        for (int ch = 0; ch < n; ++ch) {
            history_[ch].append(sample[ch]);
            if (history_[ch].size() > kHistorySamples) {
                history_[ch].remove(0, history_[ch].size() - kHistorySamples);
            }
        }
    }
    update();
}

void EegStreamView::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF bounds = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.fillRect(rect(), background_);
    painter.setPen(QPen(border_, 1.0));
    painter.drawRoundedRect(bounds, 10.0, 10.0);

    if (channelCount_ <= 0) {
        painter.setPen(muted_);
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("Load an EEG bundle to begin"));
        return;
    }

    const int visible = std::min(kVisibleChannels, channelCount_);
    if (visible <= 0 || history_.isEmpty() || history_[0].isEmpty()) {
        painter.setPen(muted_);
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("Waiting for stream…"));
        return;
    }

    const qreal padX = 14.0;
    const qreal padY = 12.0;
    const qreal plotW = width() - padX * 2.0;
    const qreal plotH = height() - padY * 2.0;
    const qreal laneH = plotH / static_cast<qreal>(visible);
    const int historyLen = history_[0].size();

    for (int ch = 0; ch < visible; ++ch) {
        const QVector<float>& series = history_[ch];
        if (series.isEmpty()) {
            continue;
        }

        float minV = series[0];
        float maxV = series[0];
        for (float v : series) {
            minV = std::min(minV, v);
            maxV = std::max(maxV, v);
        }
        if (qFuzzyCompare(minV, maxV)) {
            minV -= 1.0f;
            maxV += 1.0f;
        }

        const qreal midY = padY + laneH * (ch + 0.5);
        const qreal amp = laneH * 0.38;

        QPainterPath path;
        for (int i = 0; i < series.size(); ++i) {
            const qreal x = padX + (static_cast<qreal>(i) / std::max(1, historyLen - 1)) * plotW;
            const qreal norm = (series[i] - minV) / (maxV - minV);
            const qreal y = midY - (norm - 0.5) * 2.0 * amp;
            if (i == 0) {
                path.moveTo(x, y);
            } else {
                path.lineTo(x, y);
            }
        }

        QColor stroke = accent_;
        stroke.setAlpha(ch == 0 ? 230 : 140 - ch * 8);
        painter.setPen(QPen(stroke, 1.2));
        painter.drawPath(path);

        painter.setPen(muted_);
        painter.drawText(QRectF(padX, padY + laneH * ch, 40, 16),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("ch%1").arg(ch + 1));
    }
}
