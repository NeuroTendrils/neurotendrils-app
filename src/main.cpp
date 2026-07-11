#include "data/AppConfig.hpp"
#include "theme/AppFonts.hpp"
#include "theme/ThemeManager.hpp"
#include "ui/MainWindow.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QLibraryInfo>
#include <QMessageBox>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("NeuroTendrils"));
    app.setApplicationDisplayName(QStringLiteral("NeuroTendrils"));
    app.setOrganizationName(QStringLiteral("NeuroTendrils"));
    app.setOrganizationDomain(QStringLiteral("neurotendrils.com"));

    // RuntimeLoader needs the assimp asset importer plugin. Ensure both the
    // Qt install plugins dir and a bundled PlugIns/ folder are searchable.
    QCoreApplication::addLibraryPath(QLibraryInfo::path(QLibraryInfo::PluginsPath));
#ifdef Q_OS_MACOS
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath()
        + QStringLiteral("/../PlugIns"));
#endif

    AppFonts::initialize(app);

    AppConfig config;
    QString configError;
    if (!config.load(&configError)) {
        QMessageBox::critical(nullptr, QStringLiteral("NeuroTendrils"),
            QStringLiteral("Failed to load application data:\n%1").arg(configError));
        return 1;
    }

    ThemeManager themeManager;
    themeManager.attach(&app);

    MainWindow window(themeManager, config);
    window.show();

    return app.exec();
}
