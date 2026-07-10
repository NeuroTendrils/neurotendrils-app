#include "ui/MainWindow.h"

#include "ui/HomePage.h"

MainWindow::MainWindow(ThemeManager& themeManager, QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("NeuroTendrils"));
    resize(1280, 800);
    setMinimumSize(960, 640);

    homePage_ = new HomePage(themeManager, this);
    setCentralWidget(homePage_);
}
