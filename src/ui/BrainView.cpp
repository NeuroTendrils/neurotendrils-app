#include "ui/BrainView.hpp"

#include "theme/ColorPalette.hpp"
#include "theme/ThemeManager.hpp"

#include <QColor>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickWidget>
#include <QVariantMap>
#include <QVBoxLayout>

#include <algorithm>

BrainView::BrainView(ThemeManager& themeManager, QWidget* parent)
    : QWidget(parent)
    , themeManager_(themeManager) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quickWidget_ = new QQuickWidget(this);
    quickWidget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    connect(quickWidget_, &QQuickWidget::statusChanged, this, &BrainView::onQuickStatusChanged);

    // Properties must be set before the scene loads; setSource is async and
    // rootObject() is null until Ready, so late C++ writes are silently dropped.
    QVariantMap initial;
    initial.insert(QStringLiteral("modelSource"), modelSource_);
    quickWidget_->setInitialProperties(initial);
    quickWidget_->setSource(QUrl(QStringLiteral("qrc:/qml/BrainScene.qml")));
    layout->addWidget(quickWidget_);

    connect(&themeManager_, &ThemeManager::themeChanged, this, &BrainView::applyTheme);
}

void BrainView::onQuickStatusChanged() {
    if (quickWidget_->status() != QQuickWidget::Ready) {
        if (quickWidget_->status() == QQuickWidget::Error) {
            for (const QQmlError& error : quickWidget_->errors()) {
                qWarning("Brain scene failed to load: %s", qPrintable(error.toString()));
            }
        }
        return;
    }

    sceneReady_ = true;
    applySceneProperties();
    applyTheme();
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
    QQuickItem* scene = quickWidget_->rootObject();
    if (scene == nullptr) {
        return;
    }

    const ColorPalette colors = themeManager_.palette();
    const QColor cortex = themeManager_.effectiveTheme() == Theme::Dark
        ? QColor(QStringLiteral("#7a756c"))
        : QColor(QStringLiteral("#b5aea0"));

    scene->setProperty("backgroundColor", colors.backgroundElevated);
    scene->setProperty("baseColor", cortex);
    scene->setProperty("glowColor", colors.accent);
}
