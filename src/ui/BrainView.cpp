#include "ui/BrainView.hpp"

#include "theme/ColorPalette.hpp"
#include "theme/ThemeManager.hpp"

#include <QColor>
#include <QQuickItem>
#include <QQuickWidget>
#include <QVBoxLayout>

#include <algorithm>

BrainView::BrainView(ThemeManager& themeManager, QWidget* parent)
    : QWidget(parent)
    , themeManager_(themeManager) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quickWidget_ = new QQuickWidget(this);
    quickWidget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quickWidget_->setSource(QUrl(QStringLiteral("qrc:/qml/BrainScene.qml")));
    layout->addWidget(quickWidget_);

    if (QQuickItem* scene = quickWidget_->rootObject()) {
        scene->setProperty("modelSource", QUrl(QStringLiteral("qrc:/models/brain.glb")));
    }

    applyTheme();
    connect(&themeManager_, &ThemeManager::themeChanged, this, &BrainView::applyTheme);
}

void BrainView::setModelBounds(const QVector3D& minimum, const QVector3D& maximum) {
    QQuickItem* scene = quickWidget_->rootObject();
    if (scene == nullptr) {
        return;
    }
    const QVector3D center = (minimum + maximum) * 0.5F;
    const QVector3D span = maximum - minimum;
    const float size = std::max({span.x(), span.y(), span.z()});
    scene->setProperty("modelCenter", center);
    scene->setProperty("modelSize", size);
}

void BrainView::setHighlightIndices(const QVector<int>& modelIndices) {
    if (QQuickItem* scene = quickWidget_->rootObject()) {
        QVariantList indices;
        indices.reserve(modelIndices.size());
        for (int index : modelIndices) {
            indices.append(index);
        }
        scene->setProperty("highlightIndices", indices);
    }
}

void BrainView::clearHighlight() {
    setHighlightIndices({});
}

void BrainView::applyTheme() {
    QQuickItem* scene = quickWidget_->rootObject();
    if (scene == nullptr) {
        return;
    }

    const ColorPalette colors = themeManager_.palette();

    // The 3D stage keeps a deep, fixed backdrop in both themes so the model
    // reads with good contrast and the glow highlight stays vivid; a warm bone
    // tone gives the cortex an anatomical, non-flat look.
    scene->setProperty("backgroundColor", QColor(QStringLiteral("#0b0f1a")));
    scene->setProperty("baseColor", QColor(QStringLiteral("#c8c2b6")));
    scene->setProperty("glowColor", colors.accent);
}
