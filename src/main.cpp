#include "theme/AppFonts.h"
#include "theme/ThemeManager.h"
#include "ui/MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("NeuroTendrils"));
    app.setOrganizationName(QStringLiteral("NeuroTendrils"));
    app.setOrganizationDomain(QStringLiteral("neurotendrils.com"));

    AppFonts::initialize(app);

    ThemeManager themeManager;
    themeManager.attach(&app);

    MainWindow window(themeManager);
    window.show();

    return app.exec();
}
