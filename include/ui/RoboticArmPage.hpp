#pragma once

#include "arm/RoArmController.hpp"
#include "data/AppConfig.hpp"

#include <QWidget>

class EegStreamPlayer;
class EegStreamView;
class GestureMapper;
class ThemeManager;
class QHideEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QShowEvent;
struct EegDecodeResult;

class RoboticArmPage : public QWidget {
    Q_OBJECT

public:
    RoboticArmPage(ThemeManager& themeManager, const AppConfig& config, QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void applyTheme();
    void onBrowseBundle();
    void onPlay();
    void onPause();
    void onStop();
    void onModeToggled(bool live);
    void connectToArm();
    void onLinkChanged(RoArmController::Link link, const QString& message);
    void onPrediction(const EegDecodeResult& result);
    void onActiveMotionChanged(const QString& motionId);
    void onCommandLogged(const QString& line);
    void onSamplesBatch(const QVector<QVector<float>>& samples, qint64 startIndex);
    void onPlayerStatus(const QString& status);
    void onPlayerFinished();

private:
    void tryLoadDefaultBundle();
    bool loadBundle(const QString& path);
    void updateProgress();
    void highlightMotion(const QString& motionId);
    void setDecodeIdle();

    ThemeManager& themeManager_;
    const AppConfig& config_;

    RoArmController* arm_ = nullptr;
    EegStreamPlayer* player_ = nullptr;
    GestureMapper* mapper_ = nullptr;

    EegStreamView* streamView_ = nullptr;
    QLabel* bundleLabel_ = nullptr;
    QLabel* playerStatusLabel_ = nullptr;
    QLabel* progressLabel_ = nullptr;
    QPushButton* browseButton_ = nullptr;
    QPushButton* playButton_ = nullptr;
    QPushButton* pauseButton_ = nullptr;
    QPushButton* stopButton_ = nullptr;

    QLabel* predictedLabel_ = nullptr;
    QLabel* confidenceLabel_ = nullptr;
    QLabel* expectedLabel_ = nullptr;
    QLabel* trialLabel_ = nullptr;

    QPushButton* openChip_ = nullptr;
    QPushButton* closeChip_ = nullptr;
    QPushButton* raiseChip_ = nullptr;
    QPushButton* lowerChip_ = nullptr;

    QLabel* statusDot_ = nullptr;
    QLabel* statusText_ = nullptr;
    QPushButton* modeButton_ = nullptr;
    QLineEdit* hostEdit_ = nullptr;
    QPushButton* connectButton_ = nullptr;

    QListWidget* commandLog_ = nullptr;

    QString activeMotionId_;
    bool attemptedAutoLoad_ = false;
};
