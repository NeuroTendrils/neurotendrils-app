#include "inference/onnx_runner.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace eeg {
namespace {

Ort::SessionOptions make_session_options() {
    Ort::SessionOptions options;
    options.SetIntraOpNumThreads(1);
    options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    return options;
}

Prediction make_prediction(std::vector<float> logits, const ModelMetadata& metadata) {
    if (logits.size() != static_cast<std::size_t>(metadata.n_classes)) {
        throw std::runtime_error("Logit count does not match n_classes");
    }
    if (metadata.class_names.size() != static_cast<std::size_t>(metadata.n_classes)) {
        throw std::runtime_error("class_names size does not match n_classes");
    }

    Prediction prediction;
    prediction.logits = std::move(logits);
    prediction.class_index = static_cast<int>(
        std::max_element(prediction.logits.begin(), prediction.logits.end()) -
        prediction.logits.begin());
    prediction.class_name =
        metadata.class_names.at(static_cast<std::size_t>(prediction.class_index));

    const float max_logit =
        *std::max_element(prediction.logits.begin(), prediction.logits.end());
    prediction.probabilities.resize(prediction.logits.size());
    double sum = 0.0;
    for (std::size_t i = 0; i < prediction.logits.size(); ++i) {
        const double value = std::exp(static_cast<double>(prediction.logits[i] - max_logit));
        prediction.probabilities[i] = static_cast<float>(value);
        sum += value;
    }
    for (float& value : prediction.probabilities) {
        value = static_cast<float>(static_cast<double>(value) / sum);
    }
    return prediction;
}

}  // namespace

OnnxRunner::OnnxRunner(const std::filesystem::path& model_path, const ModelMetadata& metadata)
    : metadata_(metadata),
      env_(ORT_LOGGING_LEVEL_WARNING, "eeg-inference"),
      session_options_(make_session_options()),
#if defined(_WIN32)
      session_(env_, model_path.wstring().c_str(), session_options_),
#else
      session_(env_, model_path.c_str(), session_options_),
#endif
      input_name_(metadata.onnx.input_name),
      output_name_(metadata.onnx.output_name) {
    if (metadata_.n_chans <= 0 || metadata_.n_times <= 0 || metadata_.n_classes <= 0) {
        throw std::runtime_error("Invalid model metadata dimensions");
    }
    if (metadata_.onnx.input_layout != "NCT") {
        throw std::runtime_error("Only NCT input layout is supported");
    }

    if (session_.GetInputCount() < 1 || session_.GetOutputCount() < 1) {
        throw std::runtime_error("ONNX model must expose at least one input and output");
    }
}

const ModelMetadata& OnnxRunner::metadata() const {
    return metadata_;
}

std::vector<float> OnnxRunner::run(const std::vector<float>& input_nct) const {
    const std::size_t expected =
        static_cast<std::size_t>(metadata_.n_chans) *
        static_cast<std::size_t>(metadata_.n_times);
    if (input_nct.size() != expected) {
        std::ostringstream message;
        message << "OnnxRunner input size " << input_nct.size()
                << " does not match expected " << expected;
        throw std::runtime_error(message.str());
    }

    const std::array<int64_t, 3> input_shape = {
        1,
        static_cast<int64_t>(metadata_.n_chans),
        static_cast<int64_t>(metadata_.n_times),
    };

    Ort::MemoryInfo memory_info =
        Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        const_cast<float*>(input_nct.data()),
        input_nct.size(),
        input_shape.data(),
        input_shape.size());

    const char* input_names[] = {input_name_.c_str()};
    const char* output_names[] = {output_name_.c_str()};

    auto output_tensors = session_.Run(
        Ort::RunOptions{nullptr},
        input_names,
        &input_tensor,
        1,
        output_names,
        1);

    if (output_tensors.empty() || !output_tensors.front().IsTensor()) {
        throw std::runtime_error("ONNX Runtime returned no output tensor");
    }

    const float* output_data = output_tensors.front().GetTensorData<float>();
    const auto output_info = output_tensors.front().GetTensorTypeAndShapeInfo();
    const std::size_t output_count = output_info.GetElementCount();
    if (output_count != static_cast<std::size_t>(metadata_.n_classes)) {
        std::ostringstream message;
        message << "ONNX output size " << output_count
                << " does not match n_classes " << metadata_.n_classes;
        throw std::runtime_error(message.str());
    }

    return std::vector<float>(output_data, output_data + output_count);
}

Prediction OnnxRunner::predict(const std::vector<float>& input_nct) const {
    return make_prediction(run(input_nct), metadata_);
}

}  // namespace eeg
