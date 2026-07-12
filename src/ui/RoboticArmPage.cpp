#include "ui/RoboticArmPage.hpp"

#include "arm/RoArmController.hpp"
#include "eeg/EegBundlePaths.hpp"
#include "eeg/EegStreamPlayer.hpp"
#include "eeg/GestureMapper.hpp"
#include "theme/AppFonts.hpp"
#include "theme/ColorPalette.hpp"
#include "theme/ThemeManager.hpp"
#include "ui/EegStreamView.hpp"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QShowEvent>
#include <QVBoxLayout>

namespace {

QPushButton* makeChip(const QString& text, QWidget* parent) {
    auto* chip = new QPushButton(text, parent);
    chip->setObjectName(QStringLiteral("motion-chip"));
    chip->setCheckable(true);
    chip->setCursor(Qt::ArrowCursor);
    chip->setFont(AppFonts::semibold(12));
    chip->setMinimumHeight(34);
    chip->setFocusPolicy(Qt::NoFocus);
    return chip;
}

} // namespace

RoboticArmPage::RoboticArmPage(ThemeManager& themeManager, const AppConfig& config, QWidget* parent)
    : QWidget(parent)
    , themeManager_(themeManager)
    , config_(config) {
    setObjectName(QStringLiteral("robotic-arm-page"));
    setAttribute(Qt::WA_StyledBackground, true);

    arm_ = new RoArmController(this);
    player_ = new EegStreamPlayer(this);
    mapper_ = new GestureMapper(*arm_, config_, this);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 12, 24, 24);
    root->setSpacing(16);

    auto* title = new QLabel(QStringLiteral("Robotic Arm"), this);
    title->setObjectName(QStringLiteral("page-title"));
    title->setFont(AppFonts::semibold(28));

    auto* subtitle = new QLabel(
        QStringLiteral("Replay BNCI2014 motor imagery, decode classes, drive the RoArm."), this);
    subtitle->setObjectName(QStringLiteral("page-subtitle"));
    subtitle->setFont(AppFonts::regular(14));
    subtitle->setWordWrap(true);

    root->addWidget(title);
    root->addWidget(subtitle);

    auto* columns = new QHBoxLayout();
    columns->setSpacing(20);

    // Stream column
    auto* streamCol = new QVBoxLayout();
    streamCol->setSpacing(10);

    streamView_ = new EegStreamView(this);
    streamCol->addWidget(streamView_, 1);

    auto* bundleRow = new QHBoxLayout();
    bundleRow->setSpacing(8);
    bundleLabel_ = new QLabel(QStringLiteral("No bundle"), this);
    bundleLabel_->setObjectName(QStringLiteral("bundle-label"));
    bundleLabel_->setFont(AppFonts::regular(12));
    bundleLabel_->setWordWrap(true);
    browseButton_ = new QPushButton(QStringLiteral("Browse…"), this);
    browseButton_->setObjectName(QStringLiteral("secondary-button"));
    browseButton_->setCursor(Qt::PointingHandCursor);
    browseButton_->setFont(AppFonts::semibold(12));
    connect(browseButton_, &QPushButton::clicked, this, &RoboticArmPage::onBrowseBundle);
    bundleRow->addWidget(bundleLabel_, 1);
    bundleRow->addWidget(browseButton_);
    streamCol->addLayout(bundleRow);

    auto* transport = new QHBoxLayout();
    transport->setSpacing(8);
    playButton_ = new QPushButton(QStringLiteral("Play"), this);
    pauseButton_ = new QPushButton(QStringLiteral("Pause"), this);
    stopButton_ = new QPushButton(QStringLiteral("Stop"), this);
    for (QPushButton* button : {playButton_, pauseButton_, stopButton_}) {
        button->setObjectName(QStringLiteral("secondary-button"));
        button->setCursor(Qt::PointingHandCursor);
        button->setFont(AppFonts::semibold(12));
        button->setMinimumHeight(34);
        transport->addWidget(button);
    }
    playButton_->setObjectName(QStringLiteral("accent-button"));
    connect(playButton_, &QPushButton::clicked, this, &RoboticArmPage::onPlay);
    connect(pauseButton_, &QPushButton::clicked, this, &RoboticArmPage::onPause);
    connect(stopButton_, &QPushButton::clicked, this, &RoboticArmPage::onStop);

    playerStatusLabel_ = new QLabel(QStringLiteral("Idle"), this);
    playerStatusLabel_->setObjectName(QStringLiteral("muted-label"));
    playerStatusLabel_->setFont(AppFonts::medium(12));
    progressLabel_ = new QLabel(QStringLiteral("0 / 0"), this);
    progressLabel_->setObjectName(QStringLiteral("muted-label"));
    progressLabel_->setFont(AppFonts::regular(12));
    transport->addStretch();
    transport->addWidget(playerStatusLabel_);
    transport->addWidget(progressLabel_);
    streamCol->addLayout(transport);

    // Decode + arm column
    auto* sideCol = new QVBoxLayout();
    sideCol->setSpacing(12);

    auto* decodeHeading = new QLabel(QStringLiteral("Decode"), this);
    decodeHeading->setObjectName(QStringLiteral("section-heading"));
    decodeHeading->setFont(AppFonts::semibold(14));
    sideCol->addWidget(decodeHeading);

    predictedLabel_ = new QLabel(QStringLiteral("-"), this);
    predictedLabel_->setObjectName(QStringLiteral("predicted-label"));
    predictedLabel_->setFont(AppFonts::semibold(22));
    confidenceLabel_ = new QLabel(QStringLiteral("Confidence: -"), this);
    confidenceLabel_->setObjectName(QStringLiteral("muted-label"));
    confidenceLabel_->setFont(AppFonts::regular(13));
    expectedLabel_ = new QLabel(QStringLiteral("Expected: -"), this);
    expectedLabel_->setObjectName(QStringLiteral("muted-label"));
    expectedLabel_->setFont(AppFonts::regular(13));
    trialLabel_ = new QLabel(QStringLiteral("Trial: -"), this);
    trialLabel_->setObjectName(QStringLiteral("muted-label"));
    trialLabel_->setFont(AppFonts::regular(13));
    sideCol->addWidget(predictedLabel_);
    sideCol->addWidget(confidenceLabel_);
    sideCol->addWidget(expectedLabel_);
    sideCol->addWidget(trialLabel_);

    auto* chipRow = new QHBoxLayout();
    chipRow->setSpacing(8);
    openChip_ = makeChip(QStringLiteral("Open"), this);
    closeChip_ = makeChip(QStringLiteral("Close"), this);
    raiseChip_ = makeChip(QStringLiteral("Shoulder ↑"), this);
    lowerChip_ = makeChip(QStringLiteral("Shoulder ↓"), this);
    chipRow->addWidget(openChip_);
    chipRow->addWidget(closeChip_);
    chipRow->addWidget(raiseChip_);
    chipRow->addWidget(lowerChip_);
    // Readouts only; snap back if the user clicks a chip.
    for (QPushButton* chip : {openChip_, closeChip_, raiseChip_, lowerChip_}) {
        connect(chip, &QPushButton::clicked, this, [this]() {
            highlightMotion(activeMotionId_);
        });
    }
    sideCol->addLayout(chipRow);

    auto* armHeading = new QLabel(QStringLiteral("RoArm"), this);
    armHeading->setObjectName(QStringLiteral("section-heading"));
    armHeading->setFont(AppFonts::semibold(14));
    sideCol->addWidget(armHeading);

    auto* statusRow = new QHBoxLayout();
    statusRow->setSpacing(8);
    statusDot_ = new QLabel(this);
    statusDot_->setObjectName(QStringLiteral("status-dot"));
    statusDot_->setFixedSize(10, 10);
    statusText_ = new QLabel(QStringLiteral("Simulation mode"), this);
    statusText_->setObjectName(QStringLiteral("status-text"));
    statusText_->setFont(AppFonts::medium(12));
    statusText_->setWordWrap(true);
    modeButton_ = new QPushButton(QStringLiteral("Simulation"), this);
    modeButton_->setObjectName(QStringLiteral("mode-button"));
    modeButton_->setCheckable(true);
    modeButton_->setCursor(Qt::PointingHandCursor);
    modeButton_->setFont(AppFonts::semibold(12));
    connect(modeButton_, &QPushButton::clicked, this, &RoboticArmPage::onModeToggled);
    statusRow->addWidget(statusDot_);
    statusRow->addWidget(statusText_, 1);
    statusRow->addWidget(modeButton_);
    sideCol->addLayout(statusRow);

    auto* hostRow = new QHBoxLayout();
    hostRow->setSpacing(8);
    hostEdit_ = new QLineEdit(config_.defaultArmHost(), this);
    hostEdit_->setObjectName(QStringLiteral("host-edit"));
    hostEdit_->setFont(AppFonts::regular(12));
    hostEdit_->setPlaceholderText(QStringLiteral("e.g. 192.168.4.1"));
    hostEdit_->setClearButtonEnabled(true);
    hostEdit_->setEnabled(false);
    connectButton_ = new QPushButton(QStringLiteral("Connect"), this);
    connectButton_->setObjectName(QStringLiteral("accent-button"));
    connectButton_->setCursor(Qt::PointingHandCursor);
    connectButton_->setFont(AppFonts::semibold(12));
    connectButton_->setEnabled(false);
    connect(connectButton_, &QPushButton::clicked, this, &RoboticArmPage::connectToArm);
    connect(hostEdit_, &QLineEdit::returnPressed, this, &RoboticArmPage::connectToArm);
    hostRow->addWidget(hostEdit_, 1);
    hostRow->addWidget(connectButton_);
    sideCol->addLayout(hostRow);

    auto* logHeading = new QLabel(QStringLiteral("Command log"), this);
    logHeading->setObjectName(QStringLiteral("section-heading"));
    logHeading->setFont(AppFonts::semibold(14));
    sideCol->addWidget(logHeading);

    commandLog_ = new QListWidget(this);
    commandLog_->setObjectName(QStringLiteral("command-log"));
    commandLog_->setFont(AppFonts::regular(12));
    commandLog_->setMinimumHeight(120);
    sideCol->addWidget(commandLog_, 1);

    columns->addLayout(streamCol, 3);
    columns->addLayout(sideCol, 2);
    root->addLayout(columns, 1);

    connect(player_, &EegStreamPlayer::statusChanged, this, &RoboticArmPage::onPlayerStatus);
    connect(player_, &EegStreamPlayer::samplesBatch, this, &RoboticArmPage::onSamplesBatch);
    connect(player_, &EegStreamPlayer::prediction, this, &RoboticArmPage::onPrediction);
    connect(player_, &EegStreamPlayer::prediction, mapper_, &GestureMapper::onPrediction);
    connect(player_, &EegStreamPlayer::finished, this, &RoboticArmPage::onPlayerFinished);
    connect(mapper_, &GestureMapper::commandLogged, this, &RoboticArmPage::onCommandLogged);
    connect(mapper_, &GestureMapper::activeMotionChanged, this, &RoboticArmPage::onActiveMotionChanged);
    connect(arm_, &RoArmController::linkChanged, this, &RoboticArmPage::onLinkChanged);

    setDecodeIdle();
    connect(&themeManager_, &ThemeManager::themeChanged, this, &RoboticArmPage::applyTheme);
    applyTheme();
    onLinkChanged(arm_->link(), QStringLiteral("Simulation mode"));
}

void RoboticArmPage::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (!attemptedAutoLoad_) {
        attemptedAutoLoad_ = true;
        tryLoadDefaultBundle();
    }
}

void RoboticArmPage::hideEvent(QHideEvent* event) {
    player_->pause();
    mapper_->stopCurrent();
    QWidget::hideEvent(event);
}

void RoboticArmPage::tryLoadDefaultBundle() {
    const QString path = EegBundlePaths::defaultBundleDir();
    if (path.isEmpty()) {
        bundleLabel_->setText(QStringLiteral(
            "No EEG bundle found. Expected Resources/eeg/v2 (or set NT_EEG_BUNDLE)."));
        playerStatusLabel_->setText(QStringLiteral("Waiting for bundle"));
        return;
    }
    loadBundle(path);
}

bool RoboticArmPage::loadBundle(const QString& path) {
    QString error;
    if (!player_->load(path, &error)) {
        bundleLabel_->setText(QStringLiteral("Failed to load: %1").arg(error));
        playerStatusLabel_->setText(QStringLiteral("Load failed"));
        return false;
    }

    bundleLabel_->setText(path);
    streamView_->setChannelCount(player_->channelCount());
    streamView_->clear();
    mapper_->reset();
    setDecodeIdle();
    updateProgress();
    commandLog_->clear();
    return true;
}

void RoboticArmPage::onBrowseBundle() {
    const QString startDir = !player_->bundlePath().isEmpty()
        ? player_->bundlePath()
        : EegBundlePaths::defaultBundleDir();
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Select EEG replay bundle"),
        startDir);
    if (dir.isEmpty()) {
        return;
    }
    if (!EegBundlePaths::bundleLooksValid(dir)) {
        bundleLabel_->setText(QStringLiteral(
            "Folder is missing model_metadata.json / model.onnx / test_stream.bin"));
        return;
    }
    loadBundle(dir);
}

void RoboticArmPage::onPlay() {
    if (!player_->isLoaded()) {
        tryLoadDefaultBundle();
        if (!player_->isLoaded()) {
            return;
        }
    }
    player_->play();
}

void RoboticArmPage::onPause() {
    player_->pause();
    mapper_->stopCurrent();
}

void RoboticArmPage::onStop() {
    player_->stop();
    mapper_->reset();
    streamView_->clear();
    setDecodeIdle();
    activeMotionId_.clear();
    highlightMotion(QString());
    updateProgress();
}

void RoboticArmPage::onModeToggled(bool live) {
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

void RoboticArmPage::connectToArm() {
    const QString host = hostEdit_->text().trimmed();
    if (host.isEmpty()) {
        statusText_->setText(QStringLiteral("Enter an IP address"));
        hostEdit_->setFocus(Qt::OtherFocusReason);
        return;
    }
    arm_->setHost(host);
    connectButton_->setText(QStringLiteral("Connecting…"));
    arm_->probe();
}

void RoboticArmPage::onLinkChanged(RoArmController::Link link, const QString& message) {
    statusText_->setText(message.isEmpty() ? QStringLiteral("-") : message);
    const ColorPalette colors = themeManager_.palette();
    QColor dot = colors.foregroundMuted;
    switch (link) {
    case RoArmController::Link::Simulated:
        dot = colors.accent;
        break;
    case RoArmController::Link::Connected:
        dot = QColor(QStringLiteral("#2f9e6b"));
        connectButton_->setText(QStringLiteral("Connected"));
        break;
    case RoArmController::Link::Probing:
        dot = QColor(QStringLiteral("#c9a227"));
        connectButton_->setText(QStringLiteral("Connecting…"));
        break;
    case RoArmController::Link::Offline:
        connectButton_->setText(QStringLiteral("Connect"));
        break;
    }
    statusDot_->setStyleSheet(
        QStringLiteral("QLabel#status-dot { background-color: %1; border-radius: 5px; }")
            .arg(dot.name(QColor::HexRgb)));
}

void RoboticArmPage::onPrediction(const EegDecodeResult& result) {
    predictedLabel_->setText(result.className.isEmpty() ? QStringLiteral("-") : result.className);
    confidenceLabel_->setText(
        QStringLiteral("Confidence: %1").arg(result.confidence, 0, 'f', 3));
    expectedLabel_->setText(
        result.expectedClassName.isEmpty()
            ? QStringLiteral("Expected: -")
            : QStringLiteral("Expected: %1").arg(result.expectedClassName));
    trialLabel_->setText(QStringLiteral("Trial: %1").arg(result.trialIndex));
    updateProgress();
}

void RoboticArmPage::onActiveMotionChanged(const QString& motionId) {
    activeMotionId_ = motionId;
    highlightMotion(motionId);
}

void RoboticArmPage::onCommandLogged(const QString& line) {
    commandLog_->addItem(line);
    commandLog_->scrollToBottom();
    while (commandLog_->count() > 80) {
        delete commandLog_->takeItem(0);
    }
}

void RoboticArmPage::onSamplesBatch(const QVector<QVector<float>>& samples, qint64 startIndex) {
    Q_UNUSED(startIndex);
    streamView_->appendSamples(samples);
    updateProgress();
}

void RoboticArmPage::onPlayerStatus(const QString& status) {
    playerStatusLabel_->setText(status);
}

void RoboticArmPage::onPlayerFinished() {
    mapper_->stopCurrent();
    highlightMotion(QString());
    updateProgress();
}

void RoboticArmPage::updateProgress() {
    progressLabel_->setText(
        QStringLiteral("%1 / %2")
            .arg(player_->currentSampleIndex())
            .arg(player_->sampleCount()));
}

void RoboticArmPage::highlightMotion(const QString& motionId) {
    openChip_->setChecked(motionId == QLatin1String("hand_open"));
    closeChip_->setChecked(motionId == QLatin1String("hand_close"));
    raiseChip_->setChecked(motionId == QLatin1String("arm_raise"));
    lowerChip_->setChecked(motionId == QLatin1String("arm_lower"));
}

void RoboticArmPage::setDecodeIdle() {
    predictedLabel_->setText(QStringLiteral("-"));
    confidenceLabel_->setText(QStringLiteral("Confidence: -"));
    expectedLabel_->setText(QStringLiteral("Expected: -"));
    trialLabel_->setText(QStringLiteral("Trial: -"));
}

void RoboticArmPage::applyTheme() {
    const ColorPalette colors = themeManager_.palette();
    const QString bg = colors.background.name(QColor::HexRgb);
    const QString elevated = colors.backgroundElevated.name(QColor::HexRgb);
    const QString subtle = colors.backgroundSubtle.name(QColor::HexRgb);
    const QString fg = colors.foreground.name(QColor::HexRgb);
    const QString muted = colors.foregroundMuted.name(QColor::HexRgb);
    const QString border = colors.border.name(QColor::HexRgb);
    const QString accent = colors.accent.name(QColor::HexRgb);
    const QString accentFg = colors.accentForeground.name(QColor::HexRgb);

    streamView_->setThemeColors(subtle, fg, muted, accent, border);

    setStyleSheet(QStringLiteral(
        "QWidget#robotic-arm-page { background-color: %1; }"
        "QLabel#page-title { color: %2; background: transparent; }"
        "QLabel#page-subtitle, QLabel#muted-label, QLabel#bundle-label,"
        " QLabel#status-text { color: %3; background: transparent; }"
        "QLabel#section-heading, QLabel#predicted-label { color: %2; background: transparent; }"
        "QPushButton#secondary-button, QPushButton#mode-button {"
        "  background-color: %4; color: %2; border: 1px solid %5; border-radius: 8px;"
        "  padding: 6px 12px; }"
        "QPushButton#secondary-button:hover, QPushButton#mode-button:hover {"
        "  border-color: %6; }"
        "QPushButton#accent-button {"
        "  background-color: %6; color: %7; border: none; border-radius: 8px;"
        "  padding: 6px 14px; }"
        "QPushButton#accent-button:disabled { background-color: %5; color: %3; }"
        "QPushButton#motion-chip {"
        "  background-color: %4; color: %3; border: 1px solid %5; border-radius: 8px;"
        "  padding: 6px 10px; }"
        "QPushButton#motion-chip:checked {"
        "  background-color: %6; color: %7; border-color: %6; }"
        "QLineEdit#host-edit {"
        "  background-color: %4; color: %2; border: 1px solid %5; border-radius: 8px;"
        "  padding: 6px 10px; }"
        "QLineEdit#host-edit:focus { border-color: %6; }"
        "QLineEdit#host-edit:disabled { color: %3; }"
        "QListWidget#command-log {"
        "  background-color: %4; color: %2; border: 1px solid %5; border-radius: 10px;"
        "  padding: 4px; outline: none; }"
        "QListWidget#command-log::item { padding: 4px 6px; }")
                      .arg(bg, fg, muted, elevated, border, accent, accentFg));

    onLinkChanged(arm_->link(), statusText_ ? statusText_->text() : QString());
}
