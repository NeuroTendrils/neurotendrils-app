#pragma once

#include "arm/RoArmController.hpp"
#include "data/AppConfig.hpp"

#include <QVariantList>
#include <QVector>
#include <QWidget>

class BrainView;
class ThemeManager;
class QBoxLayout;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QResizeEvent;
class QScrollArea;
class QTimer;
class QVBoxLayout;
class RoundedBorderOverlay;

// The live interactive stage: 3D brain beside (or above) a scrollable control
// column. Layout flips between side-by-side and stacked based on window width.
class ArmWorkspace : public QWidget {
    Q_OBJECT

public:
    ArmWorkspace(ThemeManager& themeManager, const AppConfig& config, QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void applyTheme();

private:
    QWidget* buildControlColumn();
    QWidget* buildConnectionBar();
    QWidget* buildMotionControls();
    QWidget* buildLearningCard();
    QVariantList buildRegionLabelData() const;
    void updateResponsiveLayout();
    void layoutBrainBorder();

    void connectToArm();
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

    QBoxLayout* rootLayout_ = nullptr;
    QFrame* brainCard_ = nullptr;
    RoundedBorderOverlay* brainBorder_ = nullptr;
    QScrollArea* controlScroll_ = nullptr;
    QWidget* controlColumn_ = nullptr;
    QVBoxLayout* controlColumnLayout_ = nullptr;
    QWidget* learningCard_ = nullptr;
    QScrollArea* learnScroll_ = nullptr;
    BrainView* brainView_ = nullptr;
    QLabel* regionTitle_ = nullptr;

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

    bool stackedLayout_ = false;
};
