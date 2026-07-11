#pragma once

#include "data/AppConfig.hpp"

#include <QWidget>

class ArmWorkspace;
class ThemeManager;
class QLabel;
class QPushButton;
class QStackedWidget;

// The Education experience: a short guided intro that explains the brain-
// computer interface concept, then the live interactive workspace. Designed to
// stand alone as an exhibit, so a first-time visitor gets context before the
// controls.
class EducationPage : public QWidget {
    Q_OBJECT

public:
    EducationPage(ThemeManager& themeManager, const AppConfig& config, QWidget* parent = nullptr);

private slots:
    void applyTheme();

private:
    QWidget* buildOnboarding();
    void updateStep();
    void enterWorkspace();

    ThemeManager& themeManager_;
    const AppConfig& config_;

    QStackedWidget* stack_ = nullptr;
    ArmWorkspace* workspace_ = nullptr;

    QLabel* stepEyebrow_ = nullptr;
    QLabel* stepTitle_ = nullptr;
    QLabel* stepBody_ = nullptr;
    QLabel* stepProgress_ = nullptr;
    QPushButton* backStepButton_ = nullptr;
    QPushButton* nextStepButton_ = nullptr;
    QPushButton* skipButton_ = nullptr;

    int step_ = 0;
};
