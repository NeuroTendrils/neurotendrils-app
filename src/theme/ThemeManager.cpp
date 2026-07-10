#include "theme/ThemeManager.h"

#include <QApplication>
#include <QPalette>
#include <QSettings>
#include <QStyleHints>

namespace {

QString themeToString(Theme theme) {
    return theme == Theme::Light ? QStringLiteral("light") : QStringLiteral("dark");
}

Theme themeFromString(const QString& value) {
    if (value == QStringLiteral("light")) {
        return Theme::Light;
    }
    if (value == QStringLiteral("dark")) {
        return Theme::Dark;
    }
    return Theme::Dark;
}

} // namespace

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent) {}

void ThemeManager::attach(QApplication* app) {
    app_ = app;
    theme_ = resolveInitialTheme();
    applyTheme();
}

Theme ThemeManager::currentTheme() const {
    return theme_;
}

ColorPalette ThemeManager::palette() const {
    return ColorPalette::forTheme(theme_);
}

void ThemeManager::setTheme(Theme theme) {
    if (theme_ == theme) {
        return;
    }

    theme_ = theme;
    persistTheme(theme);
    applyTheme();
    emit themeChanged(theme_);
}

Theme ThemeManager::resolveInitialTheme() {
    QSettings settings;
    const QString stored = settings.value(QString::fromLatin1(kThemeStorageKey)).toString();
    if (!stored.isEmpty()) {
        return themeFromString(stored);
    }

    if (qApp != nullptr) {
        switch (qApp->styleHints()->colorScheme()) {
        case Qt::ColorScheme::Light:
            return Theme::Light;
        case Qt::ColorScheme::Dark:
            return Theme::Dark;
        case Qt::ColorScheme::Unknown:
            break;
        }
    }

    return Theme::Dark;
}

void ThemeManager::applyTheme() {
    if (app_ == nullptr) {
        return;
    }

    const ColorPalette colors = palette();

    QPalette widgetPalette;
    widgetPalette.setColor(QPalette::Window, colors.background);
    widgetPalette.setColor(QPalette::WindowText, colors.foreground);
    widgetPalette.setColor(QPalette::Base, colors.backgroundElevated);
    widgetPalette.setColor(QPalette::AlternateBase, colors.backgroundSubtle);
    widgetPalette.setColor(QPalette::Text, colors.foreground);
    widgetPalette.setColor(QPalette::Button, colors.backgroundElevated);
    widgetPalette.setColor(QPalette::ButtonText, colors.foreground);
    widgetPalette.setColor(QPalette::BrightText, colors.accentForeground);
    widgetPalette.setColor(QPalette::Highlight, colors.accent);
    widgetPalette.setColor(QPalette::HighlightedText, colors.accentForeground);
    widgetPalette.setColor(QPalette::PlaceholderText, colors.foregroundMuted);
    widgetPalette.setColor(QPalette::Link, colors.accent);
    widgetPalette.setColor(QPalette::LinkVisited, colors.accentStrong);

    app_->setPalette(widgetPalette);
    app_->setStyleSheet(buildStylesheet(colors));
}

void ThemeManager::persistTheme(Theme theme) {
    QSettings settings;
    settings.setValue(QString::fromLatin1(kThemeStorageKey), themeToString(theme));
}

QString ThemeManager::buildStylesheet(const ColorPalette& palette) const {
    const QString background = palette.background.name(QColor::HexRgb);
    const QString foreground = palette.foreground.name(QColor::HexRgb);

    return QStringLiteral(
               "QWidget {"
               "  background-color: %1;"
               "  color: %2;"
               "}"
               "QMainWindow {"
               "  background-color: %1;"
               "}")
        .arg(background, foreground);
}
