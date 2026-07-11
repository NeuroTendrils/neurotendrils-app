#include "data/AppPaths.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace AppPaths {

QString brainModelFile() {
    const QString appDir = QCoreApplication::applicationDirPath();

#if defined(Q_OS_MACOS)
    const QString bundled = QDir(appDir).absoluteFilePath(QStringLiteral("../Resources/brain.glb"));
#else
    const QString bundled = QDir(appDir).absoluteFilePath(QStringLiteral("resources/brain.glb"));
#endif

    if (QFileInfo::exists(bundled)) {
        return QFileInfo(bundled).absoluteFilePath();
    }

    // Dev fallback: running from a build tree without the install step.
#if defined(Q_OS_MACOS)
    // .../NeuroTendrils.app/Contents/MacOS → repo root is four levels up.
    const QString sourceTree = QDir(appDir).absoluteFilePath(
        QStringLiteral("../../../../assets/brain/brain.glb"));
#else
    const QString sourceTree = QDir(appDir).absoluteFilePath(
        QStringLiteral("../../assets/brain/brain.glb"));
#endif
    if (QFileInfo::exists(sourceTree)) {
        return QFileInfo(sourceTree).absoluteFilePath();
    }

    return bundled;
}

} // namespace AppPaths
