#include "ui/ArmWorkspace.hpp"

#include "theme/AppFonts.hpp"
#include "theme/ColorPalette.hpp"
#include "theme/ThemeManager.hpp"
#include "ui/BrainView.hpp"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace {

constexpr int kControlColumnWidth = 400;
constexpr int kCardRotateMs = 9000;
constexpr int kHighlightHoldMs = 5000;

const char* kIdleRegionTitle = "Hold a control to activate";
const char* kIdleRegionDetail =
    "Each motion maps to the cortical area a brain-computer interface would decode.";

} // namespace

ArmWorkspace::ArmWorkspace(ThemeManager& themeManager, const AppConfig& config, QWidget* parent)
    : QWidget(parent)
    , themeManager_(themeManager)
    , config_(config) {
    setObjectName(QStringLiteral("arm-workspace"));
    setAttribute(Qt::WA_StyledBackground, true);

    arm_ = new RoArmController(this);
    arm_->setHost(config_.defaultArmHost());
    cards_ = config_.learningCards();

    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(24, 24, 24, 24);
    rootLayout->setSpacing(24);

    auto* brainCard = new QFrame(this);
    brainCard->setObjectName(QStringLiteral("brain-card"));
    brainCard->setAttribute(Qt::WA_StyledBackground, true);
    auto* brainLayout = new QVBoxLayout(brainCard);
    brainLayout->setContentsMargins(0, 0, 0, 0);
    brainLayout->setSpacing(0);

    brainView_ = new BrainView(themeManager_, brainCard);
    brainView_->setModelBounds(config_.modelMin(), config_.modelMax());
    brainLayout->addWidget(brainView_, 1);

    auto* readout = new QWidget(brainCard);
    readout->setObjectName(QStringLiteral("brain-readout"));
    readout->setAttribute(Qt::WA_StyledBackground, true);
    auto* readoutLayout = new QVBoxLayout(readout);
    readoutLayout->setContentsMargins(20, 16, 20, 16);
    readoutLayout->setSpacing(4);

    auto* readoutLabel = new QLabel(QStringLiteral("ACTIVE REGION"), readout);
    readoutLabel->setObjectName(QStringLiteral("readout-eyebrow"));
    readoutLabel->setFont(AppFonts::semibold(10));

    regionTitle_ = new QLabel(QLatin1String(kIdleRegionTitle), readout);
    regionTitle_->setObjectName(QStringLiteral("readout-title"));
    regionTitle_->setFont(AppFonts::semibold(16));

    regionDetail_ = new QLabel(QLatin1String(kIdleRegionDetail), readout);
    regionDetail_->setObjectName(QStringLiteral("readout-detail"));
    regionDetail_->setFont(AppFonts::regular(12));
    regionDetail_->setWordWrap(true);

    readoutLayout->addWidget(readoutLabel);
    readoutLayout->addWidget(regionTitle_);
    readoutLayout->addWidget(regionDetail_);

    brainLayout->addWidget(readout);

    rootLayout->addWidget(brainCard, 3);
    rootLayout->addWidget(buildControlColumn(), 2);

    connect(arm_, &RoArmController::linkChanged, this, &ArmWorkspace::onLinkChanged);

    cardTimer_ = new QTimer(this);
    cardTimer_->setInterval(kCardRotateMs);
    connect(cardTimer_, &QTimer::timeout, this, [this]() { advanceCard(1); });
    if (!cards_.isEmpty()) {
        showCard(0);
        cardTimer_->start();
    }

    highlightClearTimer_ = new QTimer(this);
    highlightClearTimer_->setSingleShot(true);
    highlightClearTimer_->setInterval(kHighlightHoldMs);
    connect(highlightClearTimer_, &QTimer::timeout, this, &ArmWorkspace::clearActiveRegion);

    connect(&themeManager_, &ThemeManager::themeChanged, this, &ArmWorkspace::applyTheme);
    applyTheme();
    onLinkChanged(arm_->link(), QStringLiteral("Simulation mode"));
}

QWidget* ArmWorkspace::buildControlColumn() {
    auto* column = new QWidget(this);
    column->setObjectName(QStringLiteral("control-column"));
    column->setFixedWidth(kControlColumnWidth);

    auto* layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(18);

    auto* connectionBar = buildConnectionBar();
    auto* motionControls = buildMotionControls();
    auto* learningCard = buildLearningCard();

    // Connection and motion cards keep their natural height; the learning card
    // absorbs any extra vertical space so nothing gets squeezed.
    connectionBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    motionControls->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    learningCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    layout->addWidget(connectionBar);
    layout->addWidget(motionControls);
    layout->addWidget(learningCard, 1);

    return column;
}

QWidget* ArmWorkspace::buildConnectionBar() {
    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("panel-card"));
    card->setAttribute(Qt::WA_StyledBackground, true);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(12);

    auto* statusRow = new QHBoxLayout();
    statusRow->setSpacing(10);

    statusDot_ = new QLabel(card);
    statusDot_->setObjectName(QStringLiteral("status-dot"));
    statusDot_->setFixedSize(10, 10);

    statusText_ = new QLabel(QStringLiteral("Simulation mode"), card);
    statusText_->setObjectName(QStringLiteral("status-text"));
    statusText_->setFont(AppFonts::medium(13));

    modeButton_ = new QPushButton(QStringLiteral("Simulation"), card);
    modeButton_->setObjectName(QStringLiteral("mode-button"));
    modeButton_->setCheckable(true);
    modeButton_->setCursor(Qt::PointingHandCursor);
    modeButton_->setFont(AppFonts::semibold(12));
    connect(modeButton_, &QPushButton::clicked, this, &ArmWorkspace::onModeToggled);

    statusRow->addWidget(statusDot_);
    statusRow->addWidget(statusText_, 1);
    statusRow->addWidget(modeButton_);

    auto* hostRow = new QHBoxLayout();
    hostRow->setSpacing(10);

    hostEdit_ = new QLineEdit(config_.defaultArmHost(), card);
    hostEdit_->setObjectName(QStringLiteral("host-edit"));
    hostEdit_->setFont(AppFonts::regular(13));
    hostEdit_->setPlaceholderText(QStringLiteral("RoArm address"));
    hostEdit_->setEnabled(false);

    connectButton_ = new QPushButton(QStringLiteral("Connect"), card);
    connectButton_->setObjectName(QStringLiteral("connect-button"));
    connectButton_->setCursor(Qt::PointingHandCursor);
    connectButton_->setFont(AppFonts::semibold(12));
    connectButton_->setEnabled(false);
    connect(connectButton_, &QPushButton::clicked, this, [this]() {
        arm_->setHost(hostEdit_->text().trimmed());
        arm_->probe();
    });

    hostRow->addWidget(hostEdit_, 1);
    hostRow->addWidget(connectButton_);

    layout->addLayout(statusRow);
    layout->addLayout(hostRow);
    return card;
}

QWidget* ArmWorkspace::buildMotionControls() {
    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("panel-card"));
    card->setAttribute(Qt::WA_StyledBackground, true);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(12);

    auto* heading = new QLabel(QStringLiteral("Motion"), card);
    heading->setObjectName(QStringLiteral("panel-heading"));
    heading->setFont(AppFonts::semibold(15));
    layout->addWidget(heading);

    // One row per cortical region: a fixed-width region label on the left, then
    // its motion buttons. Compact and keeps the whole panel on screen.
    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(8);
    grid->setColumnMinimumWidth(0, 120);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);

    int rowIndex = 0;
    for (const MotionGroup& group : config_.motionGroups()) {
        auto* groupLabel = new QLabel(group.title, card);
        groupLabel->setObjectName(QStringLiteral("group-label"));
        groupLabel->setFont(AppFonts::medium(12));
        grid->addWidget(groupLabel, rowIndex, 0, Qt::AlignVCenter | Qt::AlignLeft);

        int columnIndex = 1;
        for (const MotionCommand& command : group.commands) {
            auto* button = new QPushButton(command.shortLabel, card);
            button->setObjectName(QStringLiteral("motion-button"));
            button->setCursor(Qt::PointingHandCursor);
            button->setFont(AppFonts::semibold(13));
            button->setFixedHeight(40);
            button->setToolTip(command.label);

            const MotionCommand captured = command;
            if (command.oneShot) {
                connect(button, &QPushButton::clicked, this, [this, captured]() {
                    beginMotion(captured);
                });
            } else {
                connect(button, &QPushButton::pressed, this, [this, captured]() {
                    beginMotion(captured);
                });
                connect(button, &QPushButton::released, this, [this, captured]() {
                    endMotion(captured);
                });
            }

            motionButtons_.append(button);
            // A lone one-shot control (Reset) spans both button columns.
            if (group.commands.size() == 1) {
                grid->addWidget(button, rowIndex, 1, 1, 2);
            } else {
                grid->addWidget(button, rowIndex, columnIndex);
            }
            ++columnIndex;
        }
        ++rowIndex;
    }

    layout->addLayout(grid);

    auto* hint = new QLabel(QStringLiteral("Hold a button to move; release to stop."), card);
    hint->setObjectName(QStringLiteral("panel-hint"));
    hint->setFont(AppFonts::regular(11));
    hint->setWordWrap(true);
    layout->addWidget(hint);

    return card;
}

QWidget* ArmWorkspace::buildLearningCard() {
    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("panel-card"));
    card->setAttribute(Qt::WA_StyledBackground, true);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(10);

    auto* headerRow = new QHBoxLayout();
    auto* heading = new QLabel(QStringLiteral("Learn"), card);
    heading->setObjectName(QStringLiteral("panel-heading"));
    heading->setFont(AppFonts::semibold(15));

    cardCategory_ = new QLabel(card);
    cardCategory_->setObjectName(QStringLiteral("card-category"));
    cardCategory_->setFont(AppFonts::semibold(10));
    cardCategory_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    headerRow->addWidget(heading);
    headerRow->addStretch();
    headerRow->addWidget(cardCategory_);

    cardTitle_ = new QLabel(card);
    cardTitle_->setObjectName(QStringLiteral("card-title"));
    cardTitle_->setFont(AppFonts::semibold(15));
    cardTitle_->setWordWrap(true);

    cardBody_ = new QLabel(card);
    cardBody_->setObjectName(QStringLiteral("card-body"));
    cardBody_->setFont(AppFonts::regular(13));
    cardBody_->setWordWrap(true);
    cardBody_->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    auto* navRow = new QHBoxLayout();
    navRow->setSpacing(10);

    auto* prev = new QPushButton(QStringLiteral("‹"), card);
    prev->setObjectName(QStringLiteral("card-nav"));
    prev->setCursor(Qt::PointingHandCursor);
    prev->setFixedSize(34, 30);
    connect(prev, &QPushButton::clicked, this, [this]() { advanceCard(-1); });

    auto* next = new QPushButton(QStringLiteral("›"), card);
    next->setObjectName(QStringLiteral("card-nav"));
    next->setCursor(Qt::PointingHandCursor);
    next->setFixedSize(34, 30);
    connect(next, &QPushButton::clicked, this, [this]() { advanceCard(1); });

    cardCounter_ = new QLabel(card);
    cardCounter_->setObjectName(QStringLiteral("card-counter"));
    cardCounter_->setFont(AppFonts::regular(11));

    navRow->addWidget(prev);
    navRow->addWidget(next);
    navRow->addStretch();
    navRow->addWidget(cardCounter_);

    layout->addLayout(headerRow);
    layout->addWidget(cardTitle_);
    layout->addWidget(cardBody_, 1);
    layout->addLayout(navRow);

    return card;
}

void ArmWorkspace::beginMotion(const MotionCommand& command) {
    setActiveRegion(command.regionId);
    // Keep the region lit while the button is held; one-shot commands have no
    // release, so their dwell countdown starts immediately.
    if (command.oneShot) {
        highlightClearTimer_->start();
    } else {
        highlightClearTimer_->stop();
    }
    arm_->sendStart(command);
}

void ArmWorkspace::endMotion(const MotionCommand& command) {
    highlightClearTimer_->start();
    arm_->sendStop(command);
}

void ArmWorkspace::setActiveRegion(const QString& regionId) {
    activeRegionId_ = regionId;
    const BrainRegion* region = config_.region(regionId);
    if (region == nullptr) {
        return;
    }

    brainView_->setHighlightIndices(region->modelIndices);
    regionTitle_->setText(region->label);
    regionDetail_->setText(region->description);
}

void ArmWorkspace::clearActiveRegion() {
    activeRegionId_.clear();
    brainView_->clearHighlight();
    regionTitle_->setText(QLatin1String(kIdleRegionTitle));
    regionDetail_->setText(QLatin1String(kIdleRegionDetail));
}

void ArmWorkspace::showCard(int index) {
    if (cards_.isEmpty()) {
        return;
    }
    cardIndex_ = (index % cards_.size() + cards_.size()) % cards_.size();
    const LearningCard& card = cards_.at(cardIndex_);
    cardCategory_->setText(card.category.toUpper());
    cardTitle_->setText(card.title);
    cardBody_->setText(card.body);
    cardCounter_->setText(QStringLiteral("%1 / %2").arg(cardIndex_ + 1).arg(cards_.size()));
}

void ArmWorkspace::advanceCard(int delta) {
    showCard(cardIndex_ + delta);
    if (cardTimer_ != nullptr) {
        cardTimer_->start(); // restart the dwell timer after any change
    }
}

void ArmWorkspace::onModeToggled() {
    const bool live = modeButton_->isChecked();
    modeButton_->setText(live ? QStringLiteral("Live") : QStringLiteral("Simulation"));
    hostEdit_->setEnabled(live);
    connectButton_->setEnabled(live);
    arm_->setMode(live ? RoArmController::Mode::Live : RoArmController::Mode::Simulation);
}

void ArmWorkspace::onLinkChanged(RoArmController::Link link, const QString& message) {
    QString color;
    switch (link) {
    case RoArmController::Link::Connected:
        color = QStringLiteral("#37c978");
        break;
    case RoArmController::Link::Simulated:
        color = QStringLiteral("#6f96ff");
        break;
    case RoArmController::Link::Probing:
        color = QStringLiteral("#f0a83c");
        break;
    case RoArmController::Link::Offline:
        color = QStringLiteral("#e5533c");
        break;
    }

    if (statusDot_ != nullptr) {
        statusDot_->setStyleSheet(
            QStringLiteral("border-radius: 5px; background-color: %1;").arg(color));
    }
    if (statusText_ != nullptr) {
        statusText_->setText(message);
    }
}

void ArmWorkspace::applyTheme() {
    const ColorPalette colors = themeManager_.palette();
    const QString bg = colors.background.name(QColor::HexRgb);
    const QString elevated = colors.backgroundElevated.name(QColor::HexRgb);
    const QString subtle = colors.backgroundSubtle.name(QColor::HexRgb);
    const QString fg = colors.foreground.name(QColor::HexRgb);
    const QString muted = colors.foregroundMuted.name(QColor::HexRgb);
    const QString border = colors.border.name(QColor::HexArgb);
    const QString accent = colors.accent.name(QColor::HexRgb);
    const QString accentSubtle = colors.accentSubtle.name(QColor::HexArgb);

    setStyleSheet(QStringLiteral(R"(
        QWidget#arm-workspace { background-color: %1; }
        QFrame#brain-card {
            background-color: %2;
            border: 1px solid %5;
            border-radius: 16px;
        }
        QWidget#brain-readout {
            background-color: %3;
            border-top: 1px solid %5;
            border-bottom-left-radius: 16px;
            border-bottom-right-radius: 16px;
        }
        QLabel#readout-eyebrow { color: %7; }
        QLabel#readout-title { color: %4; }
        QLabel#readout-detail { color: %6; }
        QFrame#panel-card {
            background-color: %2;
            border: 1px solid %5;
            border-radius: 14px;
        }
        QLabel#panel-heading { color: %4; }
        QLabel#group-label { color: %6; }
        QLabel#panel-hint, QLabel#card-counter { color: %7; }
        QLabel#status-text { color: %4; }
        QLabel#card-category { color: %8; }
        QLabel#card-title { color: %4; }
        QLabel#card-body { color: %6; }
        QPushButton#motion-button {
            background-color: %3;
            color: %4;
            border: 1px solid %5;
            border-radius: 10px;
            padding: 0 8px;
        }
        QPushButton#motion-button:hover { border-color: %8; color: %8; }
        QPushButton#motion-button:pressed { background-color: %9; border-color: %8; color: %8; }
        QLineEdit#host-edit {
            background-color: %3;
            color: %4;
            border: 1px solid %5;
            border-radius: 8px;
            padding: 6px 10px;
        }
        QLineEdit#host-edit:disabled { color: %7; }
        QPushButton#mode-button, QPushButton#connect-button, QPushButton#card-nav {
            background-color: %3;
            color: %4;
            border: 1px solid %5;
            border-radius: 8px;
            padding: 6px 14px;
        }
        QPushButton#mode-button:checked { background-color: %9; color: %8; border-color: %8; }
        QPushButton#connect-button:hover, QPushButton#card-nav:hover { border-color: %8; color: %8; }
        QPushButton#connect-button:disabled { color: %7; }
    )")
        .arg(bg, elevated, subtle, fg, border, muted, muted, accent, accentSubtle));
}
