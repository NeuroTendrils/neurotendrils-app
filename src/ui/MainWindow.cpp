#include "ui/MainWindow.hpp"

#include "theme/AppFonts.hpp"
#include "theme/ColorPalette.hpp"
#include "theme/ThemeManager.hpp"
#include "ui/EducationPage.hpp"
#include "ui/HomePage.hpp"
#include "ui/SettingsMenu.hpp"
#include "ui/SettingsOverlay.hpp"

#include <QHBoxLayout>
#include <QPushButton>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace {

constexpr int kShellPadding = 16;
constexpr int kTopBarHeight = 56;

} // namespace

MainWindow::MainWindow(ThemeManager& themeManager, const AppConfig& config, QWidget* parent)
    : QMainWindow(parent)
    , themeManager_(themeManager)
    , config_(config) {
    setWindowTitle(QStringLiteral("NeuroTendrils"));
    resize(1280, 800);
    setMinimumSize(960, 640);

    shell_ = new QWidget(this);
    shell_->setObjectName(QStringLiteral("app-shell"));
    shell_->setAttribute(Qt::WA_StyledBackground, true);

    auto* rootLayout = new QVBoxLayout(shell_);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    topBar_ = new QWidget(shell_);
    topBar_->setObjectName(QStringLiteral("top-bar"));
    topBar_->setFixedHeight(kTopBarHeight);
    topBar_->setAttribute(Qt::WA_StyledBackground, true);

    auto* topBarLayout = new QHBoxLayout(topBar_);
    topBarLayout->setContentsMargins(kShellPadding, kShellPadding, kShellPadding, 0);
    topBarLayout->setSpacing(0);

    backButton_ = new QPushButton(QStringLiteral("‹  Home"), topBar_);
    backButton_->setObjectName(QStringLiteral("back-button"));
    backButton_->setCursor(Qt::PointingHandCursor);
    backButton_->setFont(AppFonts::semibold(13));
    backButton_->hide();
    connect(backButton_, &QPushButton::clicked, this, &MainWindow::showHome);
    topBarLayout->addWidget(backButton_);

    topBarLayout->addStretch();

    settingsOverlay_ = new SettingsOverlay(themeManager_, shell_);
    settingsMenu_ = new SettingsMenu(themeManager_, settingsOverlay_, topBar_);
    topBarLayout->addWidget(settingsMenu_);

    content_ = new QStackedWidget(shell_);
    homePage_ = new HomePage(themeManager_, shell_);
    homePage_->setAttribute(Qt::WA_StyledBackground, true);
    educationPage_ = new EducationPage(themeManager_, config_, shell_);

    content_->addWidget(homePage_);
    content_->addWidget(educationPage_);

    connect(homePage_, &HomePage::educationRequested, this, &MainWindow::showEducation);

    rootLayout->addWidget(topBar_);
    rootLayout->addWidget(content_, 1);

    setCentralWidget(shell_);

    connect(&themeManager_, &ThemeManager::themeChanged, this, &MainWindow::applyTheme);
    applyTheme();
    layoutOverlay();
}

void MainWindow::showHome() {
    content_->setCurrentWidget(homePage_);
    backButton_->hide();
}

void MainWindow::showEducation() {
    content_->setCurrentWidget(educationPage_);
    backButton_->show();
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    layoutOverlay();
}

void MainWindow::layoutOverlay() {
    if (settingsOverlay_ != nullptr && shell_ != nullptr) {
        settingsOverlay_->setGeometry(shell_->rect());
    }
}

void MainWindow::applyTheme() {
    const ColorPalette colors = themeManager_.palette();
    const QString background = colors.background.name(QColor::HexRgb);
    const QString foreground = colors.foreground.name(QColor::HexRgb);
    const QString accent = colors.accent.name(QColor::HexRgb);
    const QString accentSubtle = colors.accentSubtle.name(QColor::HexArgb);

    setStyleSheet(QStringLiteral("QMainWindow { background-color: %1; }").arg(background));

    if (shell_ != nullptr) {
        shell_->setStyleSheet(QStringLiteral("QWidget#app-shell { background-color: %1; }").arg(background));
    }

    if (topBar_ != nullptr) {
        topBar_->setStyleSheet(QStringLiteral("QWidget#top-bar { background-color: %1; }").arg(background));
    }

    if (backButton_ != nullptr) {
        backButton_->setStyleSheet(QStringLiteral(
            "QPushButton#back-button {"
            "  background: transparent;"
            "  color: %1;"
            "  border: none;"
            "  padding: 6px 10px;"
            "  border-radius: 8px;"
            "}"
            "QPushButton#back-button:hover { background-color: %2; color: %3; }")
            .arg(foreground, accentSubtle, accent));
    }
}
