#include "ui/BrainView.hpp"

#include "theme/AppFonts.hpp"
#include "theme/ColorPalette.hpp"
#include "theme/ThemeManager.hpp"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickWidget>
#include <QResizeEvent>
#include <QShowEvent>
#include <QStandardPaths>
#include <QTimer>
#include <QVariantMap>
#include <QVBoxLayout>

#include <algorithm>

namespace {

// RuntimeLoader is unreliable with qrc: URLs. Staging the GLB as a real file
// makes the assimp-backed importer behave consistently across local and
// packaged builds.
QUrl stagedBrainModelUrl() {
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(cacheDir);
    const QString stagedPath = cacheDir + QStringLiteral("/brain.glb");

    QFile resource(QStringLiteral(":/models/brain.glb"));
    if (!resource.open(QIODevice::ReadOnly)) {
        qWarning("Brain model resource missing: :/models/brain.glb");
        return {};
    }

    const QByteArray bytes = resource.readAll();
    QFileInfo stagedInfo(stagedPath);
    if (!stagedInfo.exists() || stagedInfo.size() != bytes.size()) {
        QFile out(stagedPath);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning("Could not stage brain model to %s", qPrintable(stagedPath));
            return QUrl(QStringLiteral("qrc:/models/brain.glb"));
        }
        out.write(bytes);
        out.close();
    }

    return QUrl::fromLocalFile(stagedPath);
}

} // namespace

BrainView::BrainView(ThemeManager& themeManager, QWidget* parent)
    : QWidget(parent)
    , themeManager_(themeManager) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quickWidget_ = new QQuickWidget(this);
    quickWidget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quickWidget_->setClearColor(themeManager_.palette().backgroundElevated);
    connect(quickWidget_, &QQuickWidget::statusChanged, this, &BrainView::onQuickStatusChanged);
    layout->addWidget(quickWidget_);

    statusLabel_ = new QLabel(QStringLiteral("Loading atlas…"), quickWidget_);
    statusLabel_->setFont(AppFonts::medium(13));
    statusLabel_->setAlignment(Qt::AlignCenter);
    statusLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);
    statusLabel_->setGeometry(quickWidget_->rect());

    modelSource_ = stagedBrainModelUrl();

    connect(&themeManager_, &ThemeManager::themeChanged, this, &BrainView::applyTheme);
    applyTheme();
}

void BrainView::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    ensureSceneLoaded();
}

void BrainView::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (statusLabel_ != nullptr && quickWidget_ != nullptr) {
        statusLabel_->setGeometry(quickWidget_->rect());
    }
    if (isVisible() && width() > 1 && height() > 1) {
        ensureSceneLoaded();
    }
}

void BrainView::ensureSceneLoaded() {
    if (sceneRequested_ || quickWidget_ == nullptr) {
        return;
    }
    if (!isVisible() || width() < 2 || height() < 2) {
        return;
    }
    if (!modelSource_.isValid() || modelSource_.isEmpty()) {
        if (statusLabel_ != nullptr) {
            statusLabel_->setText(QStringLiteral("Brain model missing from resources"));
            statusLabel_->show();
        }
        return;
    }

    sceneRequested_ = true;
    if (statusLabel_ != nullptr) {
        statusLabel_->setText(QStringLiteral("Loading atlas…"));
        statusLabel_->show();
        statusLabel_->raise();
    }

    QVariantMap initial;
    initial.insert(QStringLiteral("modelSource"), modelSource_);
    if (hasBounds_) {
        initial.insert(QStringLiteral("modelCenter"), modelCenter_);
        initial.insert(QStringLiteral("modelSize"), modelSize_);
    }
    quickWidget_->setInitialProperties(initial);
    quickWidget_->setSource(QUrl(QStringLiteral("qrc:/qml/BrainScene.qml")));
}

void BrainView::onQuickStatusChanged() {
    if (quickWidget_->status() == QQuickWidget::Error) {
        QStringList messages;
        for (const QQmlError& error : quickWidget_->errors()) {
            messages.append(error.toString());
            qWarning("Brain scene failed to load: %s", qPrintable(error.toString()));
        }
        if (statusLabel_ != nullptr) {
            statusLabel_->setText(QStringLiteral("Brain failed to load\n%1").arg(messages.join(QLatin1Char('\n'))));
            statusLabel_->show();
            statusLabel_->raise();
        }
        return;
    }

    if (quickWidget_->status() != QQuickWidget::Ready) {
        return;
    }

    sceneReady_ = true;
    applySceneProperties();
    applyTheme();

    // RuntimeLoader finishes after the QML document is Ready — poll until the
    // scene reports ready or surfaces an import error.
    auto* poll = new QTimer(this);
    poll->setInterval(100);
    connect(poll, &QTimer::timeout, this, [this, poll]() {
        QQuickItem* scene = quickWidget_->rootObject();
        if (scene == nullptr) {
            return;
        }
        if (scene->property("ready").toBool()) {
            if (statusLabel_ != nullptr) {
                statusLabel_->hide();
            }
            poll->stop();
            poll->deleteLater();
            return;
        }
        const QString err = scene->property("loadError").toString();
        if (!err.isEmpty() && statusLabel_ != nullptr) {
            statusLabel_->setText(QStringLiteral("Brain failed to load\n%1").arg(err));
            statusLabel_->show();
            statusLabel_->raise();
            poll->stop();
            poll->deleteLater();
        }
    });
    poll->start();
}

void BrainView::setModelBounds(const QVector3D& minimum, const QVector3D& maximum) {
    modelCenter_ = (minimum + maximum) * 0.5F;
    const QVector3D span = maximum - minimum;
    modelSize_ = std::max({span.x(), span.y(), span.z()});
    hasBounds_ = modelSize_ > 0.0F;
    applySceneProperties();
}

void BrainView::setHighlightIndices(const QVector<int>& modelIndices) {
    highlightIndices_ = modelIndices;
    applySceneProperties();
}

void BrainView::clearHighlight() {
    setHighlightIndices({});
}

void BrainView::applySceneProperties() {
    if (!sceneReady_) {
        return;
    }

    QQuickItem* scene = quickWidget_->rootObject();
    if (scene == nullptr) {
        return;
    }

    scene->setProperty("modelSource", modelSource_);
    if (hasBounds_) {
        scene->setProperty("modelCenter", modelCenter_);
        scene->setProperty("modelSize", modelSize_);
    }

    QVariantList indices;
    indices.reserve(highlightIndices_.size());
    for (int index : highlightIndices_) {
        indices.append(index);
    }
    scene->setProperty("highlightIndices", indices);
}

void BrainView::applyTheme() {
    const ColorPalette colors = themeManager_.palette();
    const QColor cortex = themeManager_.effectiveTheme() == Theme::Dark
        ? QColor(QStringLiteral("#7a756c"))
        : QColor(QStringLiteral("#b5aea0"));

    if (quickWidget_ != nullptr) {
        quickWidget_->setClearColor(colors.backgroundElevated);
    }

    if (statusLabel_ != nullptr) {
        statusLabel_->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
            .arg(colors.foregroundMuted.name(QColor::HexRgb)));
    }

    QQuickItem* scene = quickWidget_ != nullptr ? quickWidget_->rootObject() : nullptr;
    if (scene == nullptr) {
        return;
    }

    scene->setProperty("backgroundColor", colors.backgroundElevated);
    scene->setProperty("baseColor", cortex);
    scene->setProperty("glowColor", colors.accent);
}
