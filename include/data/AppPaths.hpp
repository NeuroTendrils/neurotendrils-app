#pragma once

#include <QString>

namespace AppPaths {

// Absolute path to the staged brain model shipped beside the app
// (macOS: Contents/Resources/brain.glb, other platforms: resources/brain.glb).
QString brainModelFile();

} // namespace AppPaths
