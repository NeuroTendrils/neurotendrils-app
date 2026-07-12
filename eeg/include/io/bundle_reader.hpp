#pragma once

#include "common/types.hpp"

#include <filesystem>
#include <vector>

namespace eeg {

class BundleReader {
public:
    /// @param dir  Path to export bundle (e.g. export/v1).
    /// @returns  Parsed model_metadata.json.
    static ModelMetadata load_metadata(const std::filesystem::path& dir);

    /// @param dir  Path to export bundle.
    /// @returns  Path to model.onnx listed in metadata.
    static std::filesystem::path model_path(const std::filesystem::path& dir);

    /// @param dir  Path to export bundle.
    /// @returns  Path to external weights file listed in metadata.
    static std::filesystem::path external_data_path(const std::filesystem::path& dir);

    /// @param dir  Path to export bundle.
    /// @returns  Trial windows from test_trials.bin with labels from test_labels.bin.
    static std::vector<TrialWindow> load_windowed_trials(const std::filesystem::path& dir);

    /// @param dir  Path to export bundle.
    /// @returns  Parsed test_stream_manifest.json.
    static StreamManifest load_stream_manifest(const std::filesystem::path& dir);

    /// @param dir  Path to export bundle.
    /// @returns  Stream samples from test_stream.bin with trial/timestamp info.
    static std::vector<Sample> load_stream_samples(const std::filesystem::path& dir);

private:
    /// @param path  File that must exist.
    /// @returns  Nothing. Throws if missing.
    static void require_file(const std::filesystem::path& path);

    /// @param path  Path to a little-endian float32 blob.
    /// @returns  All float values in the file.
    static std::vector<float> read_float32_blob(const std::filesystem::path& path);

    /// @param path  Path to a little-endian int32 blob.
    /// @returns  All int32 values in the file.
    static std::vector<int32_t> read_int32_blob(const std::filesystem::path& path);
};

}  // namespace eeg
