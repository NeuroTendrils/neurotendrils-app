#pragma once

#include "common/types.hpp"

#include <vector>

namespace eeg {

class Preprocessor {
public:
    /// Builds FIR coefficients from metadata.preprocessing + metadata.sfreq.
    explicit Preprocessor(const ModelMetadata& metadata);

    /// Apply scale → FIR bandpass → EMS on one flat NCT window.
    /// Layout: channel-major, length n_chans * n_times.
    std::vector<float> apply(const std::vector<float>& window_nct) const;

    const PreprocessConfig& config() const { return config_; }

private:
    void design_fir_bandpass();
    void scale_inplace(std::vector<float>& window_nct) const;
    void filter_channel_inplace(std::vector<float>& channel) const;
    void ems_channel_inplace(std::vector<float>& channel) const;

    ModelMetadata metadata_;
    PreprocessConfig config_;
    std::vector<double> fir_coeffs_;
};

}  // namespace eeg
