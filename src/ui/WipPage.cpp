#include "ui/WipPage.hpp"

#include "theme/AppFonts.hpp"
#include "theme/ColorPalette.hpp"
#include "theme/ThemeManager.hpp"

#include <QLabel>
#include <QVBoxLayout>

WipPage::WipPage(ThemeManager& themeManager, const QString& featureName, QWidget* parent)
    : QWidget(parent)
    , themeManager_(themeManager)
    , featureName_(featureName) {
    setObjectName(QStringLiteral("wip-page"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(48, 48, 48, 48);
    layout->setSpacing(12);
    layout->addStretch();

    titleLabel_ = new QLabel(QStringLiteral("WIP"), this);
    titleLabel_->setObjectName(QStringLiteral("wip-title"));
    titleLabel_->setFont(AppFonts::semibold(48));
    titleLabel_->setAlignment(Qt::AlignHCenter);

    bodyLabel_ = new QLabel(this);
    bodyLabel_->setObjectName(QStringLiteral("wip-body"));
    bodyLabel_->setFont(AppFonts::regular(16));
    bodyLabel_->setAlignment(Qt::AlignHCenter);
    bodyLabel_->setWordWrap(true);

    layout->addWidget(titleLabel_);
    layout->addWidget(bodyLabel_);
    layout->addStretch();

    setFeatureName(featureName_);
    connect(&themeManager_, &ThemeManager::themeChanged, this, &WipPage::applyTheme);
    applyTheme();
}

void WipPage::setFeatureName(const QString& featureName) {
    featureName_ = featureName;
    if (bodyLabel_ != nullptr) {
        bodyLabel_->setText(QStringLiteral("%1 is still in progress.").arg(featureName_));
    }
}

void WipPage::applyTheme() {
    const ColorPalette colors = themeManager_.palette();
    const QString bg = colors.background.name(QColor::HexRgb);
    const QString fg = colors.foreground.name(QColor::HexRgb);
    const QString muted = colors.foregroundMuted.name(QColor::HexRgb);

    setStyleSheet(QStringLiteral(
        "QWidget#wip-page { background-color: %1; }"
        "QLabel#wip-title { color: %2; background: transparent; }"
        "QLabel#wip-body { color: %3; background: transparent; }")
        .arg(bg, fg, muted));
}
