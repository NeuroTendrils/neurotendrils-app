#include "theme/ThemeManager.h"

#include <QtGlobal>

static_assert(QT_VERSION >= QT_VERSION_CHECK(6, 8, 0),
              "Qt 6.8+ is required for QStyleHints::setColorScheme");

#include <QApplication>
#include <QPalette>
#include <QSettings>
#include <QStyleHints>

namespace {

QString themeToString(Theme theme) {
    switch (theme) {
    case Theme::Light:
        return QStringLiteral("light");
    case Theme::Dark:
        return QStringLiteral("dark");
    case Theme::Auto:
        return QStringLiteral("auto");
    }

    return QStringLiteral("auto");
}

Theme themeFromString(const QString& value) {
    if (value == QStringLiteral("light")) {
        return Theme::Light;
    }
    if (value == QStringLiteral("dark")) {
        return Theme::Dark;
    }
    if (value == QStringLiteral("auto")) {
        return Theme::Auto;
    }

    return Theme::Auto;
}

Theme systemTheme(QStyleHints* hints) {
    if (hints == nullptr) {
        return Theme::Dark;
    }

    switch (hints->colorScheme()) {
    case Qt::ColorScheme::Light:
        return Theme::Light;
    case Qt::ColorScheme::Dark:
        return Theme::Dark;
    case Qt::ColorScheme::Unknown:
        break;
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

    connect(app_->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        if (theme_ != Theme::Auto) {
            return;
        }

        applyTheme();
        emit themeChanged(theme_);
    });
}

Theme ThemeManager::currentTheme() const {
    return theme_;
}

Theme ThemeManager::effectiveTheme() const {
    if (theme_ == Theme::Light || theme_ == Theme::Dark) {
        return theme_;
    }

    return systemTheme(app_ != nullptr ? app_->styleHints() : nullptr);
}

ColorPalette ThemeManager::palette() const {
    return ColorPalette::forTheme(effectiveTheme());
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

    return Theme::Auto;
}

void ThemeManager::applyTheme() {
    if (app_ == nullptr) {
        return;
    }

    QStyleHints* hints = app_->styleHints();

    if (theme_ == Theme::Auto) {
        hints->setColorScheme(Qt::ColorScheme::Unknown);
    } else if (theme_ == Theme::Dark) {
        hints->setColorScheme(Qt::ColorScheme::Dark);
    } else {
        hints->setColorScheme(Qt::ColorScheme::Light);
    }

    const Theme effective = systemTheme(hints);
    const ColorPalette colors = ColorPalette::forTheme(effective);

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

    return QStringLiteral(
               "QMainWindow {"
               "  background-color: %1;"
               "}")
        .arg(background);
}
