#pragma once

#include "data/AppConfig.hpp"

#include <QObject>
#include <QString>

class RoArmController;
struct EegDecodeResult;

// Maps decoded EEG class names to RoArm hold motions with simple debounce.
// left_hand→hand_open, right_hand→hand_close, tongue→arm_raise, feet→arm_lower.
class GestureMapper : public QObject {
    Q_OBJECT

public:
    GestureMapper(RoArmController& arm, const AppConfig& config, QObject* parent = nullptr);

    void setMinConfidence(float minConfidence);
    void setStableWindows(int count);
    void reset();

public slots:
    void onPrediction(const EegDecodeResult& result);
    void stopCurrent();

signals:
    void commandLogged(const QString& line);
    void activeMotionChanged(const QString& motionId);

private:
    QString motionIdForClass(const QString& className) const;
    void applyMotion(const QString& motionId, const QString& className);

    RoArmController& arm_;
    const AppConfig& config_;
    float minConfidence_ = 0.35f;
    int stableWindowsRequired_ = 1;
    QString pendingClass_;
    int pendingCount_ = 0;
    QString activeMotionId_;
    QString activeClass_;
};
