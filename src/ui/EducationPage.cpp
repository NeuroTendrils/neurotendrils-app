#include "ui/EducationPage.hpp"

#include "theme/AppFonts.hpp"
#include "theme/ColorPalette.hpp"
#include "theme/ThemeManager.hpp"
#include "ui/ArmWorkspace.hpp"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <array>

namespace {

struct IntroStep {
    const char* eyebrow;
    const char* title;
    const char* body;
};

const std::array<IntroStep, 3> kIntroSteps = {{
    {"Step 1 of 3",
     "Thought becomes motion",
     "A brain-computer interface reads activity from the motor cortex and turns "
     "intended movement into commands for a device. Here that device is a robotic "
     "arm, and no muscles are involved: intent is decoded straight from the brain."},
    {"Step 2 of 3",
     "Every control maps to the cortex",
     "The controls on the next screen are grouped by the cortical region a real "
     "interface would decode: the hand and shoulder areas of the primary motor "
     "cortex, dorsal premotor cortex for reaching, and the supplementary motor area "
     "for sequenced poses. Hold a control and watch that region light up on the brain."},
    {"Step 3 of 3",
     "Live or simulated",
     "Simulation mode runs the full experience with no hardware attached. Switch to "
     "Live and enter the arm's address to drive a real RoArm-M2 over the network. "
     "Either way, hold a button to move and release to stop."},
}};

} // namespace

EducationPage::EducationPage(ThemeManager& themeManager, const AppConfig& config, QWidget* parent)
    : QWidget(parent)
    , themeManager_(themeManager)
    , config_(config) {
    setObjectName(QStringLiteral("education-page"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    stack_ = new QStackedWidget(this);
    // ArmWorkspace (and its Quick3D brain) is created only when the user enters
    // the interactive stage — constructing it while this page is still hidden
    // in MainWindow's stack leaves the 3D view without a usable surface.
    stack_->addWidget(buildOnboarding());
    layout->addWidget(stack_);

    updateStep();
    connect(&themeManager_, &ThemeManager::themeChanged, this, &EducationPage::applyTheme);
    applyTheme();
}

QWidget* EducationPage::buildOnboarding() {
    auto* page = new QWidget(this);
    page->setObjectName(QStringLiteral("onboarding"));
    page->setAttribute(Qt::WA_StyledBackground, true);

    auto* outer = new QHBoxLayout(page);
    outer->setContentsMargins(48, 48, 48, 48);
    outer->addStretch();

    auto* column = new QVBoxLayout();
    column->setSpacing(0);
    column->addStretch();

    auto* card = new QFrame(page);
    card->setObjectName(QStringLiteral("onboarding-card"));
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setFixedWidth(560);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(36, 32, 36, 28);
    cardLayout->setSpacing(16);

    stepEyebrow_ = new QLabel(card);
    stepEyebrow_->setObjectName(QStringLiteral("onboarding-eyebrow"));
    stepEyebrow_->setFont(AppFonts::semibold(11));

    stepTitle_ = new QLabel(card);
    stepTitle_->setObjectName(QStringLiteral("onboarding-title"));
    stepTitle_->setFont(AppFonts::semibold(26));
    stepTitle_->setWordWrap(true);

    stepBody_ = new QLabel(card);
    stepBody_->setObjectName(QStringLiteral("onboarding-body"));
    stepBody_->setFont(AppFonts::regular(15));
    stepBody_->setWordWrap(true);
    stepBody_->setMinimumHeight(140);
    stepBody_->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    auto* controls = new QHBoxLayout();
    controls->setSpacing(12);

    stepProgress_ = new QLabel(card);
    stepProgress_->setObjectName(QStringLiteral("onboarding-progress"));
    stepProgress_->setFont(AppFonts::regular(12));

    skipButton_ = new QPushButton(QStringLiteral("Skip intro"), card);
    skipButton_->setObjectName(QStringLiteral("onboarding-skip"));
    skipButton_->setCursor(Qt::PointingHandCursor);
    skipButton_->setFont(AppFonts::medium(13));
    connect(skipButton_, &QPushButton::clicked, this, &EducationPage::enterWorkspace);

    backStepButton_ = new QPushButton(QStringLiteral("Back"), card);
    backStepButton_->setObjectName(QStringLiteral("onboarding-secondary"));
    backStepButton_->setCursor(Qt::PointingHandCursor);
    backStepButton_->setFont(AppFonts::semibold(13));
    connect(backStepButton_, &QPushButton::clicked, this, [this]() {
        if (step_ > 0) {
            --step_;
            updateStep();
        }
    });

    nextStepButton_ = new QPushButton(card);
    nextStepButton_->setObjectName(QStringLiteral("onboarding-primary"));
    nextStepButton_->setCursor(Qt::PointingHandCursor);
    nextStepButton_->setFont(AppFonts::semibold(13));
    connect(nextStepButton_, &QPushButton::clicked, this, [this]() {
        if (step_ + 1 < static_cast<int>(kIntroSteps.size())) {
            ++step_;
            updateStep();
        } else {
            enterWorkspace();
        }
    });

    controls->addWidget(stepProgress_);
    controls->addStretch();
    controls->addWidget(skipButton_);
    controls->addWidget(backStepButton_);
    controls->addWidget(nextStepButton_);

    cardLayout->addWidget(stepEyebrow_);
    cardLayout->addWidget(stepTitle_);
    cardLayout->addWidget(stepBody_, 1);
    cardLayout->addLayout(controls);

    column->addWidget(card);
    column->addStretch();
    outer->addLayout(column);
    outer->addStretch();

    return page;
}

void EducationPage::updateStep() {
    const IntroStep& intro = kIntroSteps.at(step_);
    stepEyebrow_->setText(QString::fromUtf8(intro.eyebrow));
    stepTitle_->setText(QString::fromUtf8(intro.title));
    stepBody_->setText(QString::fromUtf8(intro.body));
    stepProgress_->setText(QStringLiteral("%1 / %2").arg(step_ + 1).arg(kIntroSteps.size()));

    backStepButton_->setEnabled(step_ > 0);
    const bool last = step_ + 1 == static_cast<int>(kIntroSteps.size());
    nextStepButton_->setText(last ? QStringLiteral("Start") : QStringLiteral("Next"));
}

void EducationPage::enterWorkspace() {
    if (workspace_ == nullptr) {
        workspace_ = new ArmWorkspace(themeManager_, config_, this);
        stack_->addWidget(workspace_);
    }
    stack_->setCurrentWidget(workspace_);
}

void EducationPage::applyTheme() {
    const ColorPalette colors = themeManager_.palette();
    const QString bg = colors.background.name(QColor::HexRgb);
    const QString elevated = colors.backgroundElevated.name(QColor::HexRgb);
    const QString subtle = colors.backgroundSubtle.name(QColor::HexRgb);
    const QString fg = colors.foreground.name(QColor::HexRgb);
    const QString muted = colors.foregroundMuted.name(QColor::HexRgb);
    const QString border = colors.border.name(QColor::HexArgb);
    const QString accent = colors.accent.name(QColor::HexRgb);
    const QString accentSubtle = colors.accentSubtle.name(QColor::HexArgb);
    const QString accentFg = colors.accentForeground.name(QColor::HexRgb);

    setStyleSheet(QStringLiteral(R"(
        QWidget#education-page, QWidget#onboarding { background-color: %1; }
        QFrame#onboarding-card {
            background-color: %2;
            border: 1px solid %5;
            border-radius: 18px;
        }
        QLabel#onboarding-eyebrow { color: %7; }
        QLabel#onboarding-title { color: %4; }
        QLabel#onboarding-body { color: %6; }
        QLabel#onboarding-progress { color: %7; }
        QPushButton#onboarding-skip { background: transparent; color: %7; border: none; padding: 8px; }
        QPushButton#onboarding-skip:hover { color: %8; }
        QPushButton#onboarding-secondary {
            background-color: %3; color: %4; border: 1px solid %5;
            border-radius: 9px; padding: 8px 18px;
        }
        QPushButton#onboarding-secondary:hover { border-color: %8; color: %8; }
        QPushButton#onboarding-secondary:disabled { color: %7; }
        QPushButton#onboarding-primary {
            background-color: %8; color: %9; border: none;
            border-radius: 9px; padding: 8px 22px;
        }
        QPushButton#onboarding-primary:hover { background-color: %10; }
    )")
        .arg(bg, elevated, subtle, fg, border, muted, muted, accent, accentFg)
        .arg(colors.accentStrong.name(QColor::HexRgb)));
    Q_UNUSED(accentSubtle);
}
