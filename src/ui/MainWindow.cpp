#include "ui/MainWindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("NeuroTendrils"));
    resize(1280, 800);
    setMinimumSize(960, 640);
}
