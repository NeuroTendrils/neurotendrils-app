#include "ui/SettingsMenu.hpp"

#include "theme/ColorPalette.hpp"
#include "theme/ThemeManager.hpp"
#include "ui/SettingsOverlay.hpp"

#include <QApplication>
#include <QHBoxLayout>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSvgRenderer>

namespace {

constexpr int kButtonSize = 40;
constexpr int kIconSize = 22;

QIcon tintedSvgIcon(const QString& resourcePath, const QColor& color, int logicalSize) {
    const qreal dpr = qApp != nullptr ? qApp->devicePixelRatio() : 1.0;
    const int pixelSize = qMax(1, qRound(logicalSize * dpr));

    QSvgRenderer renderer(resourcePath);
    QPixmap pixmap(pixelSize, pixelSize);
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);

    {
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        renderer.render(&painter, QRectF(0, 0, logicalSize, logicalSize));
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(QRectF(0, 0, logicalSize, logicalSize), color);
    }

    return QIcon(pixmap);
}

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

    button_ = new QPushButton(this);
    button_->setObjectName(QStringLiteral("settings-button"));
    button_->setFlat(true);
    button_->setFixedSize(kButtonSize, kButtonSize);
    button_->setIconSize(QSize(kIconSize, kIconSize));
    button_->setToolTip(QStringLiteral("Settings"));
    button_->setAccessibleName(QStringLiteral("Settings"));
    button_->setAccessibleDescription(QStringLiteral("Open application settings"));
    button_->setFocusPolicy(Qt::TabFocus);
    button_->setCursor(Qt::PointingHandCursor);

    layout->addWidget(button_);

    connect(button_, &QPushButton::clicked, this, &SettingsMenu::toggleSettings);
    connect(&themeManager_, &ThemeManager::themeChanged, this, &SettingsMenu::applyTheme);
    if (overlay_ != nullptr) {
        connect(overlay_, &SettingsOverlay::opened, this, &SettingsMenu::applyTheme);
        connect(overlay_, &SettingsOverlay::closed, this, &SettingsMenu::applyTheme);
    }

    applyTheme();
}

void SettingsMenu::applyTheme() {
    const ColorPalette colors = themeManager_.palette();
    const bool open = overlay_ != nullptr && overlay_->isOpen();
    const QColor iconColor = open ? colors.foreground : colors.foregroundMuted;

    button_->setIcon(tintedSvgIcon(QStringLiteral(":/icons/settings.svg"), iconColor, kIconSize));

    const QString accentSubtle = colors.accentSubtle.name(QColor::HexArgb);

    button_->setStyleSheet(QStringLiteral(
        "QPushButton#settings-button {"
        "  background: transparent;"
        "  border: none;"
        "  border-radius: 8px;"
        "  padding: 0;"
        "}"
        "QPushButton#settings-button:hover {"
        "  background-color: %1;"
        "}"
        "QPushButton#settings-button:pressed {"
        "  background-color: %1;"
        "}"
        "QPushButton#settings-button:focus {"
        "  outline: none;"
        "  background-color: %1;"
        "}")
        .arg(accentSubtle));
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
    applyTheme();
}
