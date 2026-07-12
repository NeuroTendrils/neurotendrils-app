#include "io/bundle_reader.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace eeg {
    namespace {

        nlohmann::json read_json_file(const std::filesystem::path& path) {
            std::ifstream input(path);
            if (!input) {
                throw std::runtime_error("Failed to open JSON file: " + path.string());
            }

            nlohmann::json document;
            input >> document;
            return document;
        }

    }  // namespace

    void BundleReader::require_file(const std::filesystem::path& path) {
        if (!std::filesystem::exists(path)) {
            throw std::runtime_error("Required export bundle file missing: " + path.string());
        }
    }

    std::vector<float> BundleReader::read_float32_blob(const std::filesystem::path& path) {
        require_file(path);

        std::ifstream input(path, std::ios::binary);
        if (!input) {
            throw std::runtime_error("Failed to open binary file: " + path.string());
        }

        input.seekg(0, std::ios::end);
        const auto byte_count = input.tellg();
        if (byte_count < 0 || byte_count % static_cast<std::streamoff>(sizeof(float)) != 0) {
            throw std::runtime_error("Invalid float32 blob size: " + path.string());
        }

        const std::size_t element_count =
            static_cast<std::size_t>(byte_count) / sizeof(float);

        std::vector<float> values(element_count);
        input.seekg(0, std::ios::beg);
        input.read(reinterpret_cast<char*>(values.data()), byte_count);

        if (!input) {
            throw std::runtime_error("Failed to read float32 blob: " + path.string());
        }

        return values;
    }

    std::vector<int32_t> BundleReader::read_int32_blob(const std::filesystem::path& path) {
        require_file(path);

        std::ifstream input(path, std::ios::binary);
        if (!input) {
            throw std::runtime_error("Failed to open binary file: " + path.string());
        }

        input.seekg(0, std::ios::end);
        const auto byte_count = input.tellg();
        if (byte_count < 0 || byte_count % static_cast<std::streamoff>(sizeof(int32_t)) != 0) {
            throw std::runtime_error("Invalid int32 blob size: " + path.string());
        }

        const std::size_t element_count =
            static_cast<std::size_t>(byte_count) / sizeof(int32_t);

        std::vector<int32_t> values(element_count);
        input.seekg(0, std::ios::beg);
        input.read(reinterpret_cast<char*>(values.data()), byte_count);

        if (!input) {
            throw std::runtime_error("Failed to read int32 blob: " + path.string());
        }

        return values;
    }

    ModelMetadata BundleReader::load_metadata(const std::filesystem::path& dir) {
        const auto path = dir / "model_metadata.json";
        const auto document = read_json_file(path);

        ModelMetadata metadata;
        metadata.n_chans = document.at("n_chans").get<int>();
        metadata.n_times = document.at("n_times").get<int>();
        metadata.sfreq = document.at("sfreq").get<float>();
        metadata.n_classes = document.at("n_classes").get<int>();
        metadata.class_names = document.at("class_names").get<std::vector<std::string>>();

        if (document.contains("onnx")) {
            const auto& onnx = document.at("onnx");
            metadata.onnx.model_file = onnx.value("model_file", metadata.onnx.model_file);
            metadata.onnx.external_data_file =
                onnx.value("external_data_file", metadata.onnx.external_data_file);
            metadata.onnx.input_name = onnx.value("input_name", metadata.onnx.input_name);
            metadata.onnx.output_name = onnx.value("output_name", metadata.onnx.output_name);
            metadata.onnx.input_layout = onnx.value("input_layout", metadata.onnx.input_layout);
            if (onnx.contains("input_shape")) {
                metadata.onnx.input_shape = onnx.at("input_shape").get<std::vector<int>>();
            }
            if (onnx.contains("output_shape")) {
                metadata.onnx.output_shape = onnx.at("output_shape").get<std::vector<int>>();
            }
        }

        if (document.contains("preprocessing")) {
            const auto& prep = document.at("preprocessing");
            metadata.preprocessing.multiply_factor =
                prep.value("multiply_factor", metadata.preprocessing.multiply_factor);
            metadata.preprocessing.low_cut_hz =
                prep.value("low_cut_hz", metadata.preprocessing.low_cut_hz);
            metadata.preprocessing.high_cut_hz =
                prep.value("high_cut_hz", metadata.preprocessing.high_cut_hz);
            metadata.preprocessing.standardize_factor_new =
                prep.value("standardize_factor_new", metadata.preprocessing.standardize_factor_new);
            metadata.preprocessing.standardize_init_block_size = prep.value(
                "standardize_init_block_size",
                metadata.preprocessing.standardize_init_block_size);
            metadata.preprocessing.standardize_eps =
                prep.value("standardize_eps", metadata.preprocessing.standardize_eps);
            metadata.preprocessing.filter_method =
                prep.value("filter_method", metadata.preprocessing.filter_method);
            metadata.preprocessing.filter_phase =
                prep.value("filter_phase", metadata.preprocessing.filter_phase);
        }

        return metadata;
    }

    std::filesystem::path BundleReader::model_path(const std::filesystem::path& dir) {
        const auto metadata = load_metadata(dir);
        const auto path = dir / metadata.onnx.model_file;
        require_file(path);
        return path;
    }

    std::filesystem::path BundleReader::external_data_path(const std::filesystem::path& dir) {
        const auto metadata = load_metadata(dir);
        if (metadata.onnx.external_data_file.empty()) {
            throw std::runtime_error("No external_data_file listed in model metadata");
        }

        const auto path = dir / metadata.onnx.external_data_file;
        require_file(path);
        return path;
    }

    std::vector<TrialWindow> BundleReader::load_windowed_trials(const std::filesystem::path& dir) {
        const auto metadata = load_metadata(dir);
        const auto flat_data = read_float32_blob(dir / "test_trials.bin");
        const auto labels = read_int32_blob(dir / "test_labels.bin");

        const std::size_t values_per_trial =
            static_cast<std::size_t>(metadata.n_chans) * static_cast<std::size_t>(metadata.n_times);

        if (flat_data.size() % values_per_trial != 0) {
            throw std::runtime_error(
                "test_trials.bin size does not align with metadata dimensions");
        }

        const std::size_t n_trials = flat_data.size() / values_per_trial;
        if (labels.size() != n_trials) {
            throw std::runtime_error(
                "test_labels.bin count (" + std::to_string(labels.size()) +
                ") does not match trial count (" + std::to_string(n_trials) + ")");
        }

        std::vector<TrialWindow> trials(n_trials);
        for (std::size_t trial_index = 0; trial_index < n_trials; trial_index++) {
            trials[trial_index].trial_index = static_cast<int>(trial_index);
            trials[trial_index].label = labels[trial_index];
            trials[trial_index].data.assign(
                flat_data.begin() + trial_index * values_per_trial,
                flat_data.begin() + (trial_index + 1) * values_per_trial);
        }

        return trials;
    }

    StreamManifest BundleReader::load_stream_manifest(const std::filesystem::path& dir) {
        const auto path = dir / "test_stream_manifest.json";
        const auto document = read_json_file(path);

        StreamManifest manifest;
        manifest.n_trials = document.at("n_trials").get<int>();
        manifest.samples_per_trial = document.at("samples_per_trial").get<int>();
        manifest.sfreq = document.at("sfreq").get<float>();
        manifest.labels = document.at("labels").get<std::vector<int>>();

        return manifest;
    }

    std::vector<Sample> BundleReader::load_stream_samples(const std::filesystem::path& dir) {
        const auto metadata = load_metadata(dir);
        const auto manifest = load_stream_manifest(dir);
        const auto flat_data = read_float32_blob(dir / "test_stream.bin");

        const std::size_t values_per_sample = static_cast<std::size_t>(metadata.n_chans);
        const std::size_t expected_samples =
            static_cast<std::size_t>(manifest.n_trials) *
            static_cast<std::size_t>(manifest.samples_per_trial);

        if (flat_data.size() != expected_samples * values_per_sample) {
            throw std::runtime_error(
                "test_stream.bin size does not match manifest dimensions");
        }

        if (static_cast<int>(manifest.labels.size()) != manifest.n_trials) {
            throw std::runtime_error("test_stream_manifest.json labels count mismatch");
        }

        std::vector<Sample> samples(expected_samples);
        const double sample_period = 1.0 / static_cast<double>(manifest.sfreq);
        std::size_t global_index = 0;

        for (int trial_index = 0; trial_index < manifest.n_trials; trial_index++) {
            for (int sample_index = 0; sample_index < manifest.samples_per_trial; sample_index++) {
                Sample& sample = samples[global_index];
                sample.trial_index = trial_index;
                sample.sample_index_in_trial = sample_index;
                sample.timestamp_seconds = static_cast<double>(global_index) * sample_period;

                const auto offset = global_index * values_per_sample;
                sample.channels.assign(
                    flat_data.begin() + static_cast<std::ptrdiff_t>(offset),
                    flat_data.begin() + static_cast<std::ptrdiff_t>(offset + values_per_sample));

                global_index++;
            }
        }

        return samples;
    }

}  // namespace eeg
