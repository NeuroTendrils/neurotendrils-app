#include "ui/MainWindow.hpp"

#include "theme/ColorPalette.hpp"
#include "theme/ThemeManager.hpp"
#include "ui/HomePage.hpp"
#include "ui/SettingsMenu.hpp"
#include "ui/SettingsOverlay.hpp"

#include <QHBoxLayout>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QWidget>

namespace {

constexpr int kShellPadding = 16;
constexpr int kTopBarHeight = 56;

} // namespace

MainWindow::MainWindow(ThemeManager& themeManager, QWidget* parent)
    : QMainWindow(parent)
    , themeManager_(themeManager) {
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
    topBarLayout->addStretch();

    settingsOverlay_ = new SettingsOverlay(themeManager_, shell_);
    settingsMenu_ = new SettingsMenu(themeManager_, settingsOverlay_, topBar_);
    topBarLayout->addWidget(settingsMenu_);

    homePage_ = new HomePage(themeManager_, shell_);
    homePage_->setAttribute(Qt::WA_StyledBackground, true);

    rootLayout->addWidget(topBar_);
    rootLayout->addWidget(homePage_, 1);

    setCentralWidget(shell_);

    connect(&themeManager_, &ThemeManager::themeChanged, this, &MainWindow::applyTheme);
    applyTheme();
    layoutOverlay();
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

    setStyleSheet(QStringLiteral("QMainWindow { background-color: %1; }").arg(background));

    if (shell_ != nullptr) {
        shell_->setStyleSheet(QStringLiteral("QWidget#app-shell { background-color: %1; }").arg(background));
    }

    if (topBar_ != nullptr) {
        topBar_->setStyleSheet(QStringLiteral("QWidget#top-bar { background-color: %1; }").arg(background));
    }
}
