#pragma once

#include <QColor>

enum class Theme {
    Light,
    Dark,
    Auto,
};

struct ColorPalette {
    QColor brandBlue;
    QColor brandPurple;

    QColor background;
    QColor backgroundSubtle;
    QColor backgroundElevated;
    QColor foreground;
    QColor foregroundMuted;

    QColor accent;
    QColor accentStrong;
    QColor accentForeground;
    QColor accentSubtle;

    QColor border;

    static ColorPalette forTheme(Theme theme);
};
