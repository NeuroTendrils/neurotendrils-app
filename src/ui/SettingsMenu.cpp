#include "ui/SettingsMenu.hpp"

#include "theme/AppFonts.hpp"
#include "theme/ColorPalette.hpp"
#include "theme/ThemeManager.hpp"
#include "ui/SettingsOverlay.hpp"

#include <QHBoxLayout>
#include <QPushButton>

namespace {

constexpr int kButtonSize = 48;
constexpr int kGearFontSize = 28;

} // namespace

SettingsMenu::SettingsMenu(
    ThemeManager& themeManager,
    SettingsOverlay* overlay,
    QWidget* parent)
    : QWidget(parent)
    , themeManager_(themeManager)
    , overlay_(overlay) {
    setFixedSize(kButtonSize, kButtonSize);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    button_ = new QPushButton(QStringLiteral("⚙"), this);
    button_->setObjectName(QStringLiteral("settings-button"));
    button_->setFlat(true);
    button_->setFixedSize(kButtonSize, kButtonSize);
    button_->setFont(AppFonts::regular(kGearFontSize));
    button_->setToolTip(QStringLiteral("Settings"));
    button_->setAccessibleName(QStringLiteral("Settings"));
    button_->setAccessibleDescription(QStringLiteral("Open application settings"));
    button_->setFocusPolicy(Qt::TabFocus);
    button_->setCursor(Qt::PointingHandCursor);

    layout->addWidget(button_);

    connect(button_, &QPushButton::clicked, this, &SettingsMenu::toggleSettings);
    connect(&themeManager_, &ThemeManager::themeChanged, this, &SettingsMenu::applyTheme);

    applyTheme();
}

void SettingsMenu::applyTheme() {
    const ColorPalette colors = themeManager_.palette();
    const QString foregroundMuted = colors.foregroundMuted.name(QColor::HexRgb);
    const QString foreground = colors.foreground.name(QColor::HexRgb);

    button_->setStyleSheet(QStringLiteral(
        "QPushButton#settings-button {"
        "  background: transparent;"
        "  border: none;"
        "  color: %1;"
        "  padding: 0;"
        "}"
        "QPushButton#settings-button:hover {"
        "  color: %2;"
        "  background: transparent;"
        "}"
        "QPushButton#settings-button:pressed {"
        "  color: %2;"
        "  background: transparent;"
        "}"
        "QPushButton#settings-button:focus {"
        "  color: %2;"
        "  outline: none;"
        "}")
        .arg(foregroundMuted, foreground));
}

void SettingsMenu::toggleSettings() {
    if (overlay_ == nullptr) {
        return;
    }

    if (overlay_->isOpen()) {
        overlay_->closePanel();
    } else {
        overlay_->openPanel();
    }
}
