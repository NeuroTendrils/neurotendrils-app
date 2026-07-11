#pragma once

#include <QMainWindow>

class HomePage;
class SettingsMenu;
class SettingsOverlay;
class ThemeManager;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(ThemeManager& themeManager, QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void applyTheme();

private:
    void layoutOverlay();

    ThemeManager& themeManager_;
    QWidget* shell_ = nullptr;
    QWidget* topBar_ = nullptr;
    HomePage* homePage_ = nullptr;
    SettingsMenu* settingsMenu_ = nullptr;
    SettingsOverlay* settingsOverlay_ = nullptr;
};
