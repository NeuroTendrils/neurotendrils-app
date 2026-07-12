#include "eeg/GestureMapper.hpp"

#include "arm/RoArmController.hpp"
#include "eeg/EegStreamPlayer.hpp"

#include <algorithm>

GestureMapper::GestureMapper(RoArmController& arm, const AppConfig& config, QObject* parent)
    : QObject(parent)
    , arm_(arm)
    , config_(config) {
}

void GestureMapper::setMinConfidence(float minConfidence) {
    minConfidence_ = minConfidence;
}

void GestureMapper::setStableWindows(int count) {
    stableWindowsRequired_ = std::max(1, count);
}

void GestureMapper::reset() {
    stopCurrent();
    pendingClass_.clear();
    pendingCount_ = 0;
}

void GestureMapper::stopCurrent() {
    if (activeMotionId_.isEmpty()) {
        return;
    }
    if (const MotionCommand* cmd = config_.motion(activeMotionId_)) {
        arm_.sendStop(*cmd);
    }
    activeMotionId_.clear();
    activeClass_.clear();
    emit activeMotionChanged(QString());
}

QString GestureMapper::motionIdForClass(const QString& className) const {
    if (className == QLatin1String("left_hand")) {
        return QStringLiteral("hand_open");
    }
    if (className == QLatin1String("right_hand")) {
        return QStringLiteral("hand_close");
    }
    if (className == QLatin1String("tongue")) {
        return QStringLiteral("arm_raise");
    }
    if (className == QLatin1String("feet")) {
        return QStringLiteral("arm_lower");
    }
    return {};
}

void GestureMapper::onPrediction(const EegDecodeResult& result) {
    const bool confident = result.confidence >= minConfidence_
        && !result.className.isEmpty();

    if (!confident) {
        pendingClass_.clear();
        pendingCount_ = 0;
        stopCurrent();
        return;
    }

    if (result.className == pendingClass_) {
        ++pendingCount_;
    } else {
        pendingClass_ = result.className;
        pendingCount_ = 1;
    }

    if (pendingCount_ < stableWindowsRequired_) {
        return;
    }

    const QString motionId = motionIdForClass(result.className);
    if (motionId.isEmpty()) {
        stopCurrent();
        return;
    }

    if (motionId == activeMotionId_) {
        return;
    }

    applyMotion(motionId, result.className);
}

void GestureMapper::applyMotion(const QString& motionId, const QString& className) {
    stopCurrent();

    const MotionCommand* cmd = config_.motion(motionId);
    if (cmd == nullptr) {
        emit commandLogged(QStringLiteral("%1 → %2 (missing command)").arg(className, motionId));
        return;
    }

    arm_.sendStart(*cmd);
    activeMotionId_ = motionId;
    activeClass_ = className;
    emit activeMotionChanged(motionId);
    emit commandLogged(QStringLiteral("%1 → %2").arg(className, motionId));
}
