#pragma once

#include <QWidget>

class QPushButton;
class SettingsOverlay;
class ThemeManager;

class SettingsMenu final : public QWidget {
    Q_OBJECT

public:
    SettingsMenu(ThemeManager& themeManager, SettingsOverlay* overlay, QWidget* parent = nullptr);

private slots:
    void applyTheme();
    void toggleSettings();

private:
    ThemeManager& themeManager_;
    SettingsOverlay* overlay_ = nullptr;
    QPushButton* button_ = nullptr;
};
