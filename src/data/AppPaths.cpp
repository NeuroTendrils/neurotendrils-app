#include "data/AppPaths.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace AppPaths {

namespace {

QString firstExistingFile(const QStringList& candidates) {
    for (const QString& path : candidates) {
        if (path.isEmpty()) {
            continue;
        }
        const QFileInfo info(path);
        if (info.isFile()) {
            return info.absoluteFilePath();
        }
    }
    return {};
}

} // namespace

QString brainModelFile() {
    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates;

    // Staged install layouts (POST_BUILD / packagers).
    candidates.append(QDir(appDir).absoluteFilePath(QStringLiteral("resources/brain.glb")));
    candidates.append(QDir(appDir).absoluteFilePath(QStringLiteral("../Resources/brain.glb"))); // macOS .app
    candidates.append(QDir(appDir).absoluteFilePath(QStringLiteral("../share/neurotendrils/brain.glb")));

    // Dev builds: walk up from the binary toward the source tree.
    const QStringList relativeDev = {
        QStringLiteral("assets/brain/brain.glb"),
        QStringLiteral("../assets/brain/brain.glb"),
        QStringLiteral("../../assets/brain/brain.glb"),
        QStringLiteral("../../../assets/brain/brain.glb"),
        QStringLiteral("../../../../assets/brain/brain.glb"),
        QStringLiteral("../../../../../assets/brain/brain.glb"),
    };
    for (const QString& relative : relativeDev) {
        candidates.append(QDir(appDir).absoluteFilePath(relative));
    }

    const QString found = firstExistingFile(candidates);
    if (!found.isEmpty()) {
        return found;
    }

    // Prefer the staged path in error messages even if missing.
    return candidates.constFirst();
}

} // namespace AppPaths
