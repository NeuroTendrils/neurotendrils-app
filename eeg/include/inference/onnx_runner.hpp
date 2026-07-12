#pragma once

#include "common/types.hpp"

#include <onnxruntime_cxx_api.h>

#include <filesystem>
#include <string>
#include <vector>

namespace eeg {

class OnnxRunner {
public:
    /// Loads model.onnx (and sibling external weights if present).
    OnnxRunner(const std::filesystem::path& model_path, const ModelMetadata& metadata);

    OnnxRunner(const OnnxRunner&) = delete;
    OnnxRunner& operator=(const OnnxRunner&) = delete;

    /// Forward pass. @param input_nct flat NCT, length n_chans * n_times.
    /// @returns class logits, length n_classes.
    std::vector<float> run(const std::vector<float>& input_nct) const;

    /// Forward pass plus argmax + softmax probabilities.
    Prediction predict(const std::vector<float>& input_nct) const;

    const ModelMetadata& metadata() const;

private:
    ModelMetadata metadata_;
    Ort::Env env_;
    Ort::SessionOptions session_options_;
    mutable Ort::Session session_;
    std::string input_name_;
    std::string output_name_;
};

}  // namespace eeg
