#include "arm/RoArmController.hpp"

#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

namespace {

constexpr int kRequestTimeoutMs = 4000;

QUrl commandUrl(const QString& host, const QJsonObject& payload) {
    QUrl url;
    url.setScheme(QStringLiteral("http"));
    url.setHost(host);
    url.setPath(QStringLiteral("/js"));

    QUrlQuery query;
    query.addQueryItem(
        QStringLiteral("json"),
        QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact)));
    url.setQuery(query);
    return url;
}

} // namespace

RoArmController::RoArmController(QObject* parent)
    : QObject(parent)
    , network_(new QNetworkAccessManager(this)) {}

void RoArmController::setHost(const QString& host) {
    host_ = host;
}

void RoArmController::setMode(Mode mode) {
    if (mode_ == mode) {
        return;
    }
    mode_ = mode;

    if (mode_ == Mode::Simulation) {
        setLink(Link::Simulated, QStringLiteral("Simulation mode"));
    } else {
        setLink(Link::Offline, QStringLiteral("Not connected"));
        probe();
    }
}

void RoArmController::probe() {
    if (mode_ == Mode::Simulation) {
        setLink(Link::Simulated, QStringLiteral("Simulation mode"));
        return;
    }
    if (host_.isEmpty()) {
        setLink(Link::Offline, QStringLiteral("No arm address set"));
        return;
    }

    setLink(Link::Probing, QStringLiteral("Connecting to %1…").arg(host_));

    // T:405 is a lightweight status query on the RoArm firmware.
    QNetworkRequest request(commandUrl(host_, QJsonObject{{QStringLiteral("T"), 405}}));
    request.setTransferTimeout(kRequestTimeoutMs);
    QNetworkReply* reply = network_->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (mode_ != Mode::Live) {
            return;
        }
        if (reply->error() == QNetworkReply::NoError) {
            setLink(Link::Connected, QStringLiteral("Connected to %1").arg(host_));
        } else {
            setLink(Link::Offline, QStringLiteral("Unreachable: %1").arg(reply->errorString()));
        }
    });
}

void RoArmController::sendStart(const MotionCommand& command) {
    sendPayload(command.id, command.startPayload, true);
}

void RoArmController::sendStop(const MotionCommand& command) {
    if (command.oneShot || command.stopPayload.isEmpty()) {
        return;
    }
    sendPayload(command.id, command.stopPayload, false);
}

void RoArmController::sendPayload(const QString& motionId, const QJsonObject& payload, bool started) {
    if (payload.isEmpty()) {
        return;
    }

    if (mode_ == Mode::Simulation) {
        emit commandDispatched(motionId, started);
        return;
    }

    if (host_.isEmpty()) {
        emit commandFailed(motionId, QStringLiteral("No arm address set"));
        return;
    }

    QNetworkRequest request(commandUrl(host_, payload));
    request.setTransferTimeout(kRequestTimeoutMs);
    QNetworkReply* reply = network_->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, motionId, started]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit commandDispatched(motionId, started);
            if (link_ != Link::Connected) {
                setLink(Link::Connected, QStringLiteral("Connected to %1").arg(host_));
            }
        } else {
            emit commandFailed(motionId, reply->errorString());
            setLink(Link::Offline, QStringLiteral("Lost connection: %1").arg(reply->errorString()));
        }
    });
}

void RoArmController::setLink(Link link, const QString& message) {
    if (link_ == link) {
        return;
    }
    link_ = link;
    emit linkChanged(link_, message);
}
