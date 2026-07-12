#include "ui/ArmWorkspace.hpp"

#include "theme/AppFonts.hpp"
#include "theme/ColorPalette.hpp"
#include "theme/ThemeManager.hpp"
#include "ui/BrainView.hpp"

#include <QBoxLayout>
#include <QColor>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSizePolicy>
#include <QTimer>
#include <QVariantMap>
#include <QVBoxLayout>

namespace {

constexpr int kControlColumnMinWidth = 280;
constexpr int kControlColumnMaxWidthWide = 520;
constexpr int kWideBreakpoint = 1100;
constexpr int kCardRotateMs = 9000;
constexpr int kHighlightHoldMs = 5000;
constexpr int kBrainMinHeightStacked = 220;
constexpr qreal kBrainCardRadius = 16.0;

const char* kIdleRegionTitle = "Hold a control to activate";

} // namespace

// Draws a clean rounded stroke on top of Quick3D / child widgets so stylesheet
// borders aren't painted over at the corners.
class RoundedBorderOverlay final : public QWidget {
public:
    explicit RoundedBorderOverlay(QWidget* parent = nullptr)
        : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_NoSystemBackground);
    }

    void setBorderColor(const QColor& color) {
        if (color_ == color) {
            return;
        }
        color_ = color;
        update();
    }

    void setRadius(qreal radius) {
        radius_ = radius;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setBrush(Qt::NoBrush);
        QPen pen(color_, 1.0);
        pen.setJoinStyle(Qt::RoundJoin);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), radius_, radius_);
    }

private:
    QColor color_{QStringLiteral("#d8dde8")};
    qreal radius_ = 16.0;
};

ArmWorkspace::ArmWorkspace(ThemeManager& themeManager, const AppConfig& config, QWidget* parent)
    : QWidget(parent)
    , themeManager_(themeManager)
    , config_(config) {
    setObjectName(QStringLiteral("arm-workspace"));
    setAttribute(Qt::WA_StyledBackground, true);

    arm_ = new RoArmController(this);
    arm_->setHost(config_.defaultArmHost());
    cards_ = config_.learningCards();

    rootLayout_ = new QBoxLayout(QBoxLayout::LeftToRight, this);
    rootLayout_->setContentsMargins(24, 24, 24, 24);
    rootLayout_->setSpacing(24);

    brainCard_ = new QFrame(this);
    brainCard_->setObjectName(QStringLiteral("brain-card"));
    brainCard_->setAttribute(Qt::WA_StyledBackground, true);
    brainCard_->setMinimumWidth(240);
    brainCard_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* brainLayout = new QVBoxLayout(brainCard_);
    brainLayout->setContentsMargins(0, 0, 0, 0);
    brainLayout->setSpacing(0);

    brainView_ = new BrainView(themeManager_, brainCard_);
    brainView_->setModelBounds(config_.modelMin(), config_.modelMax());
    brainView_->setRegionLabels(buildRegionLabelData());
    brainLayout->addWidget(brainView_, 1);

    auto* readout = new QWidget(brainCard_);
    readout->setObjectName(QStringLiteral("brain-readout"));
    readout->setAttribute(Qt::WA_StyledBackground, true);
    auto* readoutLayout = new QHBoxLayout(readout);
    readoutLayout->setContentsMargins(16, 10, 16, 10);
    readoutLayout->setSpacing(0);

    regionTitle_ = new QLabel(QLatin1String(kIdleRegionTitle), readout);
    regionTitle_->setObjectName(QStringLiteral("readout-title"));
    regionTitle_->setFont(AppFonts::medium(13));
    regionTitle_->setWordWrap(false);
    regionTitle_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    readoutLayout->addWidget(regionTitle_, 1);

    brainLayout->addWidget(readout);

    // Paint the rounded stroke above Quick3D so corners stay continuous.
    brainBorder_ = new RoundedBorderOverlay(brainCard_);
    brainBorder_->setRadius(kBrainCardRadius);
    brainBorder_->raise();
    brainCard_->installEventFilter(this);

    controlScroll_ = new QScrollArea(this);
    controlScroll_->setObjectName(QStringLiteral("control-scroll"));
    controlScroll_->setWidgetResizable(true);
    controlScroll_->setFrameShape(QFrame::NoFrame);
    controlScroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    controlScroll_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    controlColumn_ = buildControlColumn();
    controlScroll_->setWidget(controlColumn_);

    rootLayout_->addWidget(brainCard_, 3);
    rootLayout_->addWidget(controlScroll_, 2);

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
    updateResponsiveLayout();
}

QWidget* ArmWorkspace::buildControlColumn() {
    auto* column = new QWidget();
    column->setObjectName(QStringLiteral("control-column"));
    column->setMinimumWidth(kControlColumnMinWidth);
    column->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    controlColumnLayout_ = new QVBoxLayout(column);
    controlColumnLayout_->setContentsMargins(0, 0, 4, 0);
    controlColumnLayout_->setSpacing(14);

    auto* connectionBar = buildConnectionBar();
    motionCard_ = buildMotionControls();
    learningCard_ = buildLearningCard();

    connectionBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    motionCard_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    learningCard_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    learningCard_->setMinimumHeight(140);
    learningCard_->setMaximumHeight(240);

    controlColumnLayout_->addWidget(connectionBar);
    controlColumnLayout_->addWidget(motionCard_, 1);
    controlColumnLayout_->addWidget(learningCard_, 0);

    return column;
}

QWidget* ArmWorkspace::buildConnectionBar() {
    auto* card = new QFrame();
    card->setObjectName(QStringLiteral("panel-card"));
    card->setAttribute(Qt::WA_StyledBackground, true);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);

    auto* statusRow = new QHBoxLayout();
    statusRow->setSpacing(8);

    statusDot_ = new QLabel(card);
    statusDot_->setObjectName(QStringLiteral("status-dot"));
    statusDot_->setFixedSize(10, 10);

    statusText_ = new QLabel(QStringLiteral("Simulation mode"), card);
    statusText_->setObjectName(QStringLiteral("status-text"));
    statusText_->setFont(AppFonts::medium(12));
    statusText_->setWordWrap(true);

    modeButton_ = new QPushButton(QStringLiteral("Simulation"), card);
    modeButton_->setObjectName(QStringLiteral("mode-button"));
    modeButton_->setCheckable(true);
    modeButton_->setCursor(Qt::PointingHandCursor);
    modeButton_->setFont(AppFonts::semibold(12));
    modeButton_->setToolTip(QStringLiteral("Toggle between simulation and a live RoArm"));
    connect(modeButton_, &QPushButton::clicked, this, &ArmWorkspace::onModeToggled);

    statusRow->addWidget(statusDot_);
    statusRow->addWidget(statusText_, 1);
    statusRow->addWidget(modeButton_);

    auto* hostRow = new QHBoxLayout();
    hostRow->setSpacing(8);

    hostEdit_ = new QLineEdit(config_.defaultArmHost(), card);
    hostEdit_->setObjectName(QStringLiteral("host-edit"));
    hostEdit_->setFont(AppFonts::regular(12));
    hostEdit_->setPlaceholderText(QStringLiteral("e.g. 192.168.4.1"));
    hostEdit_->setClearButtonEnabled(true);
    hostEdit_->setEnabled(false);
    hostEdit_->setToolTip(QStringLiteral("RoArm IP address or hostname"));

    connectButton_ = new QPushButton(QStringLiteral("Connect"), card);
    connectButton_->setObjectName(QStringLiteral("connect-button"));
    connectButton_->setCursor(Qt::PointingHandCursor);
    connectButton_->setFont(AppFonts::semibold(12));
    connectButton_->setEnabled(false);
    connectButton_->setDefault(true);
    connect(connectButton_, &QPushButton::clicked, this, &ArmWorkspace::connectToArm);
    connect(hostEdit_, &QLineEdit::returnPressed, this, &ArmWorkspace::connectToArm);
    connect(hostEdit_, &QLineEdit::textEdited, this, [this](const QString&) {
        if (modeButton_ != nullptr && modeButton_->isChecked()
            && arm_ != nullptr && arm_->link() == RoArmController::Link::Connected) {
            statusText_->setText(QStringLiteral("Address changed; press Connect"));
            connectButton_->setText(QStringLiteral("Connect"));
        }
    });

    hostRow->addWidget(hostEdit_, 1);
    hostRow->addWidget(connectButton_);

    layout->addLayout(statusRow);
    layout->addLayout(hostRow);
    return card;
}

QWidget* ArmWorkspace::buildMotionControls() {
    auto* card = new QFrame();
    card->setObjectName(QStringLiteral("panel-card"));
    card->setAttribute(Qt::WA_StyledBackground, true);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);

    auto* heading = new QLabel(QStringLiteral("Motion"), card);
    heading->setObjectName(QStringLiteral("panel-heading"));
    heading->setFont(AppFonts::semibold(14));
    layout->addWidget(heading);

    motionGrid_ = new QGridLayout();
    motionGrid_->setHorizontalSpacing(8);
    motionGrid_->setVerticalSpacing(8);
    motionGrid_->setColumnMinimumWidth(0, 88);
    motionGrid_->setColumnStretch(1, 1);
    motionGrid_->setColumnStretch(2, 1);

    int rowIndex = 0;
    for (const MotionGroup& group : config_.motionGroups()) {
        auto* groupLabel = new QLabel(group.title, card);
        groupLabel->setObjectName(QStringLiteral("group-label"));
        groupLabel->setFont(AppFonts::medium(11));
        groupLabel->setWordWrap(true);
        motionGrid_->addWidget(groupLabel, rowIndex, 0, Qt::AlignVCenter | Qt::AlignLeft);
        motionGrid_->setRowStretch(rowIndex, 1);

        int columnIndex = 1;
        for (const MotionCommand& command : group.commands) {
            auto* button = new QPushButton(command.shortLabel, card);
            button->setObjectName(QStringLiteral("motion-button"));
            button->setCursor(Qt::PointingHandCursor);
            button->setFont(AppFonts::semibold(12));
            button->setMinimumHeight(36);
            button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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
            if (group.commands.size() == 1) {
                motionGrid_->addWidget(button, rowIndex, 1, 1, 2);
            } else {
                motionGrid_->addWidget(button, rowIndex, columnIndex);
            }
            ++columnIndex;
        }
        ++rowIndex;
    }

    layout->addLayout(motionGrid_, 1);

    auto* hint = new QLabel(QStringLiteral("Hold a button to move; release to stop."), card);
    hint->setObjectName(QStringLiteral("panel-hint"));
    hint->setFont(AppFonts::regular(11));
    hint->setWordWrap(true);
    layout->addWidget(hint);

    return card;
}

QWidget* ArmWorkspace::buildLearningCard() {
    auto* card = new QFrame();
    card->setObjectName(QStringLiteral("panel-card"));
    card->setAttribute(Qt::WA_StyledBackground, true);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
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

    cardBody_ = new QLabel();
    cardBody_->setObjectName(QStringLiteral("card-body"));
    cardBody_->setFont(AppFonts::regular(13));
    cardBody_->setWordWrap(true);
    cardBody_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    cardBody_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    learnScroll_ = new QScrollArea(card);
    learnScroll_->setObjectName(QStringLiteral("learn-scroll"));
    learnScroll_->setWidgetResizable(true);
    learnScroll_->setFrameShape(QFrame::NoFrame);
    learnScroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    learnScroll_->setWidget(cardBody_);
    learnScroll_->setMinimumHeight(72);
    learnScroll_->setMaximumHeight(120);
    learnScroll_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    auto* navRow = new QHBoxLayout();
    navRow->setSpacing(8);

    auto* prev = new QPushButton(QStringLiteral("‹"), card);
    prev->setObjectName(QStringLiteral("card-nav"));
    prev->setCursor(Qt::PointingHandCursor);
    prev->setFixedSize(34, 28);
    connect(prev, &QPushButton::clicked, this, [this]() { advanceCard(-1); });

    auto* next = new QPushButton(QStringLiteral("›"), card);
    next->setObjectName(QStringLiteral("card-nav"));
    next->setCursor(Qt::PointingHandCursor);
    next->setFixedSize(34, 28);
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
    layout->addWidget(learnScroll_, 0);
    layout->addLayout(navRow);

    return card;
}

QVariantList ArmWorkspace::buildRegionLabelData() const {
    QVariantList labels;
    labels.reserve(config_.regions().size());

    for (const BrainRegion& region : config_.regions()) {
        QStringList motionNames;
        for (const MotionGroup& group : config_.motionGroups()) {
            for (const MotionCommand& command : group.commands) {
                if (command.regionId == region.id) {
                    motionNames.append(command.shortLabel.isEmpty() ? command.label : command.shortLabel);
                }
            }
        }

        QVariantList indices;
        indices.reserve(region.modelIndices.size());
        for (int index : region.modelIndices) {
            indices.append(index);
        }

        QVariantMap entry;
        entry.insert(QStringLiteral("id"), region.id);
        entry.insert(
            QStringLiteral("title"),
            region.shortLabel.isEmpty() ? region.label : region.shortLabel);
        entry.insert(QStringLiteral("description"), region.description);
        entry.insert(QStringLiteral("color"), region.color.name(QColor::HexRgb));
        entry.insert(QStringLiteral("motions"), motionNames.join(QStringLiteral(" / ")));
        entry.insert(QStringLiteral("indices"), indices);
        if (region.hasAnchor) {
            entry.insert(QStringLiteral("anchor"), QVariant::fromValue(region.anchorLocal));
        }
        labels.append(entry);
    }

    return labels;
}

void ArmWorkspace::updateResponsiveLayout() {
    if (rootLayout_ == nullptr || brainCard_ == nullptr || controlScroll_ == nullptr) {
        return;
    }

    const bool stacked = width() < kWideBreakpoint;
    const int pad = stacked ? 12 : (width() < 1280 ? 18 : 28);
    const int gap = stacked ? 12 : (width() < 1280 ? 20 : 28);
    rootLayout_->setContentsMargins(pad, pad, pad, pad);
    rootLayout_->setSpacing(gap);

    stackedLayout_ = stacked;

    if (stacked) {
        rootLayout_->setDirection(QBoxLayout::TopToBottom);
        rootLayout_->setStretch(0, 3);
        rootLayout_->setStretch(1, 2);
        brainCard_->setMinimumHeight(kBrainMinHeightStacked);
        brainCard_->setMaximumWidth(QWIDGETSIZE_MAX);
        controlScroll_->setMinimumWidth(0);
        controlScroll_->setMaximumWidth(QWIDGETSIZE_MAX);
        controlScroll_->setMinimumHeight(180);
        controlScroll_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        if (controlColumn_ != nullptr) {
            controlColumn_->setMinimumWidth(0);
            controlColumn_->setMaximumWidth(QWIDGETSIZE_MAX);
        }
        if (learnScroll_ != nullptr) {
            learnScroll_->setMaximumHeight(96);
        }
        if (learningCard_ != nullptr) {
            learningCard_->setMaximumHeight(220);
        }
    } else {
        rootLayout_->setDirection(QBoxLayout::LeftToRight);
        // Give the control column a real share of wide windows instead of
        // capping it and leaving empty gutter space.
        rootLayout_->setStretch(0, 5);
        rootLayout_->setStretch(1, 4);
        brainCard_->setMinimumHeight(0);
        brainCard_->setMaximumWidth(QWIDGETSIZE_MAX);

        const int available = qMax(0, width() - pad * 2 - gap);
        const int controlTarget = qBound(340, (available * 4) / 9, kControlColumnMaxWidthWide);
        controlScroll_->setMinimumWidth(controlTarget);
        controlScroll_->setMaximumWidth(controlTarget);
        controlScroll_->setMinimumHeight(0);
        controlScroll_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        if (controlColumn_ != nullptr) {
            controlColumn_->setMinimumWidth(controlTarget - 8);
            controlColumn_->setMaximumWidth(QWIDGETSIZE_MAX);
        }
        if (learnScroll_ != nullptr) {
            learnScroll_->setMaximumHeight(120);
        }
        if (learningCard_ != nullptr) {
            learningCard_->setMaximumHeight(240);
        }
        if (controlColumnLayout_ != nullptr) {
            controlColumnLayout_->setSpacing(width() >= 1400 ? 18 : 14);
        }
    }

    updateMotionButtonSizes();
}

void ArmWorkspace::updateMotionButtonSizes() {
    if (motionButtons_.isEmpty()) {
        return;
    }

    // Extra vertical room goes to motion controls, not Learn.
    const int buttonHeight = qBound(36, height() / 18, 64);
    const int pointSize = buttonHeight >= 52 ? 14 : 12;
    for (QPushButton* button : motionButtons_) {
        button->setMinimumHeight(buttonHeight);
        button->setFont(AppFonts::semibold(pointSize));
    }
    if (motionGrid_ != nullptr) {
        motionGrid_->setVerticalSpacing(buttonHeight >= 48 ? 10 : 8);
    }
}

void ArmWorkspace::connectToArm() {
    if (arm_ == nullptr || hostEdit_ == nullptr || connectButton_ == nullptr) {
        return;
    }
    if (!modeButton_->isChecked()) {
        return;
    }
    if (arm_->link() == RoArmController::Link::Probing) {
        return;
    }

    const QString host = hostEdit_->text().trimmed();
    if (host.isEmpty()) {
        statusText_->setText(QStringLiteral("Enter an arm address first"));
        hostEdit_->setFocus(Qt::OtherFocusReason);
        return;
    }

    arm_->setHost(host);
    connectButton_->setEnabled(false);
    connectButton_->setText(QStringLiteral("Connecting…"));
    arm_->probe();
}

void ArmWorkspace::beginMotion(const MotionCommand& command) {
    setActiveRegion(command.regionId);
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

    brainView_->setActiveRegionId(regionId);
    brainView_->setHighlightIndices(region->modelIndices);
    regionTitle_->setText(region->label);
}

void ArmWorkspace::clearActiveRegion() {
    activeRegionId_.clear();
    brainView_->clearHighlight();
    regionTitle_->setText(QLatin1String(kIdleRegionTitle));
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
        cardTimer_->start();
    }
}

void ArmWorkspace::onModeToggled() {
    const bool live = modeButton_->isChecked();
    modeButton_->setText(live ? QStringLiteral("Live") : QStringLiteral("Simulation"));
    hostEdit_->setEnabled(live);
    connectButton_->setEnabled(live);
    connectButton_->setText(QStringLiteral("Connect"));
    arm_->setMode(live ? RoArmController::Mode::Live : RoArmController::Mode::Simulation);
    if (live) {
        hostEdit_->setFocus(Qt::OtherFocusReason);
        hostEdit_->selectAll();
    }
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

    const bool live = modeButton_ != nullptr && modeButton_->isChecked();
    if (connectButton_ != nullptr) {
        const bool probing = link == RoArmController::Link::Probing;
        connectButton_->setEnabled(live && !probing);
        if (probing) {
            connectButton_->setText(QStringLiteral("Connecting…"));
        } else if (link == RoArmController::Link::Connected) {
            connectButton_->setText(QStringLiteral("Reconnect"));
        } else {
            connectButton_->setText(QStringLiteral("Connect"));
        }
    }
}

void ArmWorkspace::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateResponsiveLayout();
    layoutBrainBorder();
}

bool ArmWorkspace::eventFilter(QObject* watched, QEvent* event) {
    if (watched == brainCard_ && event->type() == QEvent::Resize) {
        layoutBrainBorder();
    }
    return QWidget::eventFilter(watched, event);
}

void ArmWorkspace::layoutBrainBorder() {
    if (brainBorder_ == nullptr || brainCard_ == nullptr) {
        return;
    }
    brainBorder_->setGeometry(brainCard_->rect());
    brainBorder_->raise();
}

void ArmWorkspace::applyTheme() {
    const ColorPalette colors = themeManager_.palette();
    const QString bg = colors.background.name(QColor::HexRgb);
    const QString elevated = colors.backgroundElevated.name(QColor::HexRgb);
    const QString subtle = colors.backgroundSubtle.name(QColor::HexRgb);
    const QString fg = colors.foreground.name(QColor::HexRgb);
    const QString muted = colors.foregroundMuted.name(QColor::HexRgb);
    const QString border = colors.border.name(QColor::HexRgb);
    const QString accent = colors.accent.name(QColor::HexRgb);
    const QString accentSubtle = colors.accentSubtle.name(QColor::HexArgb);
    const QString accentFg = colors.accentForeground.name(QColor::HexRgb);

    setStyleSheet(QStringLiteral(R"(
        QWidget#arm-workspace { background-color: %1; }
        QFrame#brain-card {
            background-color: %2;
            border: none;
            border-radius: 16px;
        }
        QWidget#brain-readout {
            background-color: %3;
            border: none;
            border-top: 1px solid %5;
            border-bottom-left-radius: 16px;
            border-bottom-right-radius: 16px;
        }
        QLabel#readout-title { color: %4; }
        QFrame#panel-card {
            background-color: %2;
            border: 1px solid %5;
            border-radius: 12px;
        }
        QLabel#panel-heading { color: %4; }
        QLabel#group-label { color: %6; }
        QLabel#panel-hint, QLabel#card-counter { color: %7; }
        QLabel#status-text { color: %4; }
        QLabel#card-category { color: %8; }
        QLabel#card-title { color: %4; }
        QLabel#card-body { color: %6; background: transparent; }
        QScrollArea#control-scroll,
        QScrollArea#learn-scroll {
            background: transparent;
            border: none;
        }
        QScrollArea#control-scroll > QWidget > QWidget,
        QScrollArea#learn-scroll > QWidget > QWidget { background: transparent; }
        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: %5;
            border-radius: 4px;
            min-height: 24px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QPushButton#motion-button {
            background-color: %3;
            color: %4;
            border: 1px solid %5;
            border-radius: 9px;
            padding: 0 8px;
        }
        QPushButton#motion-button:hover { border-color: %8; color: %8; }
        QPushButton#motion-button:pressed { background-color: %9; border-color: %8; color: %8; }
        QLineEdit#host-edit {
            background-color: %3;
            color: %4;
            border: 1px solid %5;
            border-radius: 8px;
            padding: 5px 10px;
            selection-background-color: %8;
        }
        QLineEdit#host-edit:focus { border-color: %8; }
        QLineEdit#host-edit:disabled { color: %7; }
        QPushButton#mode-button, QPushButton#card-nav {
            background-color: %3;
            color: %4;
            border: 1px solid %5;
            border-radius: 8px;
            padding: 5px 12px;
        }
        QPushButton#mode-button:checked { background-color: %9; color: %8; border-color: %8; }
        QPushButton#connect-button {
            background-color: %8;
            color: %10;
            border: none;
            border-radius: 8px;
            padding: 5px 12px;
            min-width: 88px;
        }
        QPushButton#connect-button:hover { background-color: %11; }
        QPushButton#connect-button:disabled {
            background-color: %3;
            color: %7;
            border: 1px solid %5;
        }
        QPushButton#card-nav:hover { border-color: %8; color: %8; }
    )")
        .arg(bg, elevated, subtle, fg, border, muted, muted, accent, accentSubtle)
        .arg(accentFg, colors.accentStrong.name(QColor::HexRgb)));

    if (brainBorder_ != nullptr) {
        brainBorder_->setBorderColor(colors.border);
        layoutBrainBorder();
    }
}
