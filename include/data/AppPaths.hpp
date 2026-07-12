#pragma once

#include <QString>

namespace AppPaths {

// Absolute path to the staged brain model shipped beside the app
// (resources/brain.glb, or Contents/Resources/brain.glb inside a macOS .app).
QString brainModelFile();

} // namespace AppPaths
