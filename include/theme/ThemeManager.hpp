#pragma once

#include "theme/ColorPalette.hpp"

#include <QObject>

class QApplication;

class ThemeManager final : public QObject {
    Q_OBJECT

public:
    static constexpr const char* kThemeStorageKey = "nt-theme";

    explicit ThemeManager(QObject* parent = nullptr);

    void attach(QApplication* app);
    Theme currentTheme() const;
    Theme effectiveTheme() const;
    ColorPalette palette() const;

    void setTheme(Theme theme);

    static Theme resolveInitialTheme();

signals:
    void themeChanged(Theme theme);

private:
    void applyTheme();
    void persistTheme(Theme theme);
    QString buildStylesheet(const ColorPalette& palette) const;

    QApplication* app_ = nullptr;
    Theme theme_ = Theme::Auto;
};
