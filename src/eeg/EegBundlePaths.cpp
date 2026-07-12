#include "eeg/EegBundlePaths.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace EegBundlePaths {

namespace {

bool hasRequiredFiles(const QString& dir) {
    const QDir root(dir);
    return root.exists(QStringLiteral("model_metadata.json"))
        && root.exists(QStringLiteral("model.onnx"))
        && root.exists(QStringLiteral("test_stream.bin"))
        && root.exists(QStringLiteral("test_stream_manifest.json"));
}

QString firstExisting(const QStringList& candidates) {
    for (const QString& path : candidates) {
        if (path.isEmpty()) {
            continue;
        }
        const QFileInfo info(path);
        if (info.isDir() && hasRequiredFiles(info.absoluteFilePath())) {
            return info.absoluteFilePath();
        }
    }
    return {};
}

} // namespace

QString defaultBundleDir() {
    QStringList candidates;

    const QByteArray env = qgetenv("NT_EEG_BUNDLE");
    if (!env.isEmpty()) {
        candidates.append(QString::fromLocal8Bit(env));
    }

    const QString appDir = QCoreApplication::applicationDirPath();

    // Staged install layouts (POST_BUILD / packagers).
    candidates.append(QDir(appDir).absoluteFilePath(QStringLiteral("resources/eeg/v2")));
    candidates.append(QDir(appDir).absoluteFilePath(QStringLiteral("../Resources/eeg/v2"))); // macOS .app
    candidates.append(QDir(appDir).absoluteFilePath(QStringLiteral("../share/neurotendrils/eeg/v2")));

    // Dev builds: walk up from the binary toward the source tree.
    // Covers flat build/, multi-config build/Release/, and macOS .app nesting.
    const QStringList relativeDev = {
        QStringLiteral("assets/eeg/v2"),
        QStringLiteral("../assets/eeg/v2"),
        QStringLiteral("../../assets/eeg/v2"),
        QStringLiteral("../../../assets/eeg/v2"),
        QStringLiteral("../../../../assets/eeg/v2"),
        QStringLiteral("../../../../../assets/eeg/v2"),
    };
    for (const QString& relative : relativeDev) {
        candidates.append(QDir(appDir).absoluteFilePath(relative));
    }

    return firstExisting(candidates);
}

bool bundleLooksValid(const QString& dir) {
    return !dir.isEmpty() && hasRequiredFiles(dir);
}

} // namespace EegBundlePaths
