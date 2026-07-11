#pragma once

#include <QMainWindow>

class AppConfig;
class EducationPage;
class HomePage;
class SettingsMenu;
class SettingsOverlay;
class ThemeManager;
class QPushButton;
class QStackedWidget;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(ThemeManager& themeManager, const AppConfig& config, QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void applyTheme();

private:
    void layoutOverlay();
    void showHome();
    void showEducation();

    ThemeManager& themeManager_;
    const AppConfig& config_;
    QWidget* shell_ = nullptr;
    QWidget* topBar_ = nullptr;
    QPushButton* backButton_ = nullptr;
    QStackedWidget* content_ = nullptr;
    HomePage* homePage_ = nullptr;
    EducationPage* educationPage_ = nullptr;
    SettingsMenu* settingsMenu_ = nullptr;
    SettingsOverlay* settingsOverlay_ = nullptr;
};
