#include "ui/HomePage.h"

#include "theme/ColorPalette.h"
#include "theme/ThemeManager.h"

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QVBoxLayout>

namespace {

constexpr int kWordmarkMinPointSize = 40;
constexpr int kWordmarkMaxPointSize = 80;

} // namespace

HomePage::HomePage(ThemeManager& themeManager, QWidget* parent)
    : QWidget(parent)
    , themeManager_(themeManager) {
    buildUi();
    applyTheme();
    updateWordmarkFont();

    connect(&themeManager_, &ThemeManager::themeChanged, this, &HomePage::applyTheme);
}

void HomePage::buildUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    rootLayout->addStretch(1);

    auto* wordmarkRow = new QHBoxLayout();
    wordmarkRow->setSpacing(0);
    wordmarkRow->addStretch();

    neuroLabel_ = new QLabel(QStringLiteral("Neuro"), this);
    neuroLabel_->setObjectName(QStringLiteral("wordmark-neuro"));

    tendrilsLabel_ = new QLabel(QStringLiteral("Tendrils"), this);
    tendrilsLabel_->setObjectName(QStringLiteral("wordmark-tendrils"));

    wordmarkRow->addWidget(neuroLabel_);
    wordmarkRow->addWidget(tendrilsLabel_);
    wordmarkRow->addStretch();

    rootLayout->addLayout(wordmarkRow);
    rootLayout->addStretch(6);
}

void HomePage::applyTheme() {
    const ColorPalette colors = themeManager_.palette();
    const QString background = colors.background.name(QColor::HexRgb);

    setStyleSheet(QStringLiteral("QWidget { background-color: %1; }").arg(background));

    if (neuroLabel_ != nullptr) {
        neuroLabel_->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
            .arg(colors.accent.name(QColor::HexRgb)));
    }

    if (tendrilsLabel_ != nullptr) {
        tendrilsLabel_->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
            .arg(colors.foreground.name(QColor::HexRgb)));
    }
}

void HomePage::updateWordmarkFont() {
    QFont wordmarkFont = font();
    const int scaledSize = qBound(kWordmarkMinPointSize, width() / 16, kWordmarkMaxPointSize);
    wordmarkFont.setPointSize(scaledSize);
    wordmarkFont.setWeight(QFont::DemiBold);
    wordmarkFont.setLetterSpacing(QFont::AbsoluteSpacing, -2.0);

    if (neuroLabel_ != nullptr) {
        neuroLabel_->setFont(wordmarkFont);
    }

    if (tendrilsLabel_ != nullptr) {
        tendrilsLabel_->setFont(wordmarkFont);
    }
}

void HomePage::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateWordmarkFont();
}
