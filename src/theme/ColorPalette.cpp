#include "theme/ColorPalette.h"

ColorPalette ColorPalette::forTheme(Theme theme) {
    ColorPalette palette;
    palette.brandBlue = QColor(QStringLiteral("#2b59c3"));
    palette.brandPurple = QColor(QStringLiteral("#7d44b1"));

    if (theme == Theme::Light) {
        palette.background = QColor(QStringLiteral("#ffffff"));
        palette.backgroundSubtle = QColor(QStringLiteral("#f4f6fb"));
        palette.backgroundElevated = QColor(QStringLiteral("#ffffff"));
        palette.foreground = QColor(QStringLiteral("#1a1f36"));
        palette.foregroundMuted = QColor(QStringLiteral("#5c6478"));
        palette.accent = palette.brandBlue;
        palette.accentStrong = palette.brandPurple;
        palette.accentForeground = QColor(QStringLiteral("#ffffff"));
        palette.accentSubtle = QColor(43, 89, 195, 20); // ~8% alpha
        palette.border = QColor(26, 31, 54, 26);          // ~10% alpha
        return palette;
    }

    palette.background = QColor(QStringLiteral("#0a0d14"));
    palette.backgroundSubtle = QColor(QStringLiteral("#0e1220"));
    palette.backgroundElevated = QColor(QStringLiteral("#12172a"));
    palette.foreground = QColor(QStringLiteral("#e7eaf2"));
    palette.foregroundMuted = QColor(QStringLiteral("#97a0b5"));
    palette.accent = QColor(QStringLiteral("#6f96ff"));
    palette.accentStrong = QColor(QStringLiteral("#a184e8"));
    palette.accentForeground = QColor(QStringLiteral("#ffffff"));
    palette.accentSubtle = QColor(111, 150, 255, 31); // ~12% alpha
    palette.border = QColor(231, 234, 242, 23);        // ~9% alpha
    return palette;
}
