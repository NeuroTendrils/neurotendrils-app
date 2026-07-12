#pragma once

#include <QString>

namespace EegBundlePaths {

// Prefer NT_EEG_BUNDLE, else staged resources/eeg/v2 (or macOS Resources),
// else a source-tree assets/eeg/v2 walk-up for local builds.
QString defaultBundleDir();

bool bundleLooksValid(const QString& dir);

} // namespace EegBundlePaths
