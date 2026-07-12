#pragma once

#include <string>
#include <vector>

namespace eeg {

struct OnnxMetadata {
    std::string model_file = "model.onnx";
    std::string external_data_file;
    std::string input_name = "x";
    std::string output_name = "linear";
    std::string input_layout = "NCT";
    std::vector<int> input_shape;
    std::vector<int> output_shape;
};

/// Matches NeuroTendrils/model configs/default.json preprocessing.
struct PreprocessConfig {
    float multiply_factor = 1.0e6f;
    float low_cut_hz = 4.0f;
    float high_cut_hz = 40.0f;
    float standardize_factor_new = 0.001f;
    int standardize_init_block_size = 1000;
    float standardize_eps = 1.0e-4f;
    std::string filter_method = "fir";
    std::string filter_phase = "zero";
};

struct ModelMetadata {
    int n_chans = 0;
    int n_times = 0;
    float sfreq = 0.0f;
    int n_classes = 0;
    std::vector<std::string> class_names;
    OnnxMetadata onnx;
    PreprocessConfig preprocessing;
};

struct TrialWindow {
    std::vector<float> data;
    int label = -1;
    int trial_index = 0;
};

struct Sample {
    std::vector<float> channels;
    int trial_index = 0;
    int sample_index_in_trial = 0;
    double timestamp_seconds = 0.0;
};

struct StreamManifest {
    int n_trials = 0;
    int samples_per_trial = 0;
    float sfreq = 0.0f;
    std::vector<int> labels;
};

struct Prediction {
    int class_index = -1;
    std::string class_name;
    std::vector<float> logits;
    std::vector<float> probabilities;
};

}  // namespace eeg
