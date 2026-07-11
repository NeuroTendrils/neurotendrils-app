#pragma once

#include "arm/RoArmController.hpp"
#include "data/AppConfig.hpp"

#include <QVector>
#include <QWidget>

class BrainView;
class ThemeManager;
class QButtonGroup;
class QLabel;
class QLineEdit;
class QPushButton;
class QTimer;

// The live interactive stage: 3D brain on the left, a control column on the
// right with connection controls, hold-to-move motion buttons grouped by the
// cortical region they map to, and a rotating deck of learning cards.
class ArmWorkspace : public QWidget {
    Q_OBJECT

public:
    ArmWorkspace(ThemeManager& themeManager, const AppConfig& config, QWidget* parent = nullptr);

private slots:
    void applyTheme();

private:
    QWidget* buildControlColumn();
    QWidget* buildConnectionBar();
    QWidget* buildMotionControls();
    QWidget* buildLearningCard();

    void beginMotion(const MotionCommand& command);
    void endMotion(const MotionCommand& command);
    void setActiveRegion(const QString& regionId);
    void clearActiveRegion();
    void showCard(int index);
    void advanceCard(int delta);

    void onLinkChanged(RoArmController::Link link, const QString& message);
    void onModeToggled();

    ThemeManager& themeManager_;
    const AppConfig& config_;
    RoArmController* arm_ = nullptr;

    BrainView* brainView_ = nullptr;
    QLabel* regionTitle_ = nullptr;
    QLabel* regionDetail_ = nullptr;

    QPushButton* modeButton_ = nullptr;
    QLineEdit* hostEdit_ = nullptr;
    QPushButton* connectButton_ = nullptr;
    QLabel* statusDot_ = nullptr;
    QLabel* statusText_ = nullptr;

    QLabel* cardCategory_ = nullptr;
    QLabel* cardTitle_ = nullptr;
    QLabel* cardBody_ = nullptr;
    QLabel* cardCounter_ = nullptr;
    QTimer* cardTimer_ = nullptr;
    QVector<LearningCard> cards_;
    int cardIndex_ = 0;

    QVector<QPushButton*> motionButtons_;
    QString activeRegionId_;
    QTimer* highlightClearTimer_ = nullptr;
};
