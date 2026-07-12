#pragma once

#include "data/AppConfig.hpp"

#include <QJsonObject>
#include <QObject>
#include <QString>

class QNetworkAccessManager;

// Talks to the Waveshare RoArm-M2 over its HTTP `/js` endpoint. A Simulation
// mode runs the exact same command flow with no network traffic so the
// Education experience works on a laptop or at an event with no hardware.
class RoArmController : public QObject {
    Q_OBJECT

public:
    enum class Mode {
        Simulation,
        Live,
    };
    Q_ENUM(Mode)

    enum class Link {
        Offline,     // Live mode, not yet probed / unreachable
        Probing,     // Live mode, checking reachability
        Connected,   // Live mode, arm responded
        Simulated,   // Simulation mode, always "connected"
    };
    Q_ENUM(Link)

    explicit RoArmController(QObject* parent = nullptr);

    Mode mode() const { return mode_; }
    Link link() const { return link_; }
    QString host() const { return host_; }

    void setHost(const QString& host);

public slots:
    void setMode(Mode mode);
    void probe();
    void sendStart(const MotionCommand& command);
    void sendStop(const MotionCommand& command);

signals:
    void linkChanged(RoArmController::Link link, const QString& message);
    void commandDispatched(const QString& motionId, bool started);
    void commandFailed(const QString& motionId, const QString& error);

private:
    void setLink(Link link, const QString& message);
    void sendPayload(const QString& motionId, const QJsonObject& payload, bool started);

    QNetworkAccessManager* network_ = nullptr;
    Mode mode_ = Mode::Simulation;
    Link link_ = Link::Simulated;
    QString host_;
    QString lastMessage_;
};
