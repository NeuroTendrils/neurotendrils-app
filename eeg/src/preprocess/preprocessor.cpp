#include "preprocess/preprocessor.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace eeg {
namespace {

constexpr double kPi = 3.14159265358979323846;

int next_odd(int value) {
    return value % 2 == 0 ? value + 1 : value;
}

/// MNE/scipy firwin Hamming bandpass, matching braindecode FIR defaults.
std::vector<double> design_firwin_bandpass(
    float sfreq,
    float low_cut_hz,
    float high_cut_hz) {
    if (sfreq <= 0.0f) {
        throw std::runtime_error("sfreq must be positive");
    }
    if (!(low_cut_hz > 0.0f && high_cut_hz > low_cut_hz)) {
        throw std::runtime_error("Invalid bandpass cutoffs");
    }

    const double nyquist = 0.5 * static_cast<double>(sfreq);
    if (high_cut_hz >= static_cast<float>(nyquist)) {
        throw std::runtime_error("high_cut_hz must be below Nyquist");
    }

    // MNE auto transition bandwidths.
    const double l_trans = std::min(
        std::max(static_cast<double>(low_cut_hz) * 0.25, 2.0),
        static_cast<double>(low_cut_hz));
    const double h_trans = std::min(
        std::max(static_cast<double>(high_cut_hz) * 0.25, 2.0),
        nyquist - static_cast<double>(high_cut_hz));
    const double width = std::min(l_trans, h_trans);

    // Hamming firwin length ≈ 3.3 / (width/sfreq), rounded up to odd.
    int num_taps = static_cast<int>(std::ceil(3.3 * sfreq / width));
    num_taps = next_odd(std::max(num_taps, 3));

    const double f1 = (static_cast<double>(low_cut_hz) - l_trans / 2.0) / nyquist;
    const double f2 = (static_cast<double>(high_cut_hz) + h_trans / 2.0) / nyquist;
    if (!(0.0 < f1 && f1 < f2 && f2 < 1.0)) {
        throw std::runtime_error("Normalized FIR cutoffs out of range");
    }

    std::vector<double> coeffs(static_cast<std::size_t>(num_taps), 0.0);
    const int m = num_taps - 1;
    const double mid = 0.5 * m;

    for (int n = 0; n < num_taps; ++n) {
        const double x = static_cast<double>(n) - mid;
        double ideal = 0.0;
        if (std::abs(x) < 1e-12) {
            ideal = f2 - f1;
        } else {
            ideal = (std::sin(kPi * f2 * x) - std::sin(kPi * f1 * x)) / (kPi * x);
        }
        const double window = 0.54 - 0.46 * std::cos(2.0 * kPi * n / m);
        coeffs[static_cast<std::size_t>(n)] = ideal * window;
    }

    // Normalize passband gain near geometric center.
    const double f0 = 0.5 * (f1 + f2);
    double gain = 0.0;
    for (int n = 0; n < num_taps; ++n) {
        const double x = static_cast<double>(n) - mid;
        gain += coeffs[static_cast<std::size_t>(n)] * std::cos(kPi * f0 * x);
    }
    if (std::abs(gain) > 1e-12) {
        for (double& c : coeffs) {
            c /= gain;
        }
    }

    return coeffs;
}

std::vector<double> lfilter_forward(
    const std::vector<double>& b,
    double a1_inv_alpha,
    const std::vector<double>& input) {
    // y[n] = b[0]*x[n] + b[1]*x[n-1] + ...  - a1 * y[n-1]
    // For EMS we use b=[1], a=[1, -(1-alpha)] so a1 = -(1-alpha) → feedback *(1-alpha)
    std::vector<double> output(input.size(), 0.0);
    const std::size_t order = b.size();
    double y_prev = 0.0;
    for (std::size_t n = 0; n < input.size(); ++n) {
        double acc = 0.0;
        for (std::size_t k = 0; k < order; ++k) {
            if (n >= k) {
                acc += b[k] * input[n - k];
            }
        }
        if (n > 0) {
            acc += a1_inv_alpha * y_prev;
        }
        output[n] = acc;
        y_prev = acc;
    }
    return output;
}

std::vector<float> convolve_same(
    const std::vector<float>& input,
    const std::vector<double>& kernel) {
    const int n = static_cast<int>(input.size());
    const int k = static_cast<int>(kernel.size());
    const int pad = k / 2;

    std::vector<float> padded(static_cast<std::size_t>(n + 2 * pad));
    for (int i = 0; i < pad; ++i) {
        const int src = std::min(pad - i, n - 1);
        padded[static_cast<std::size_t>(i)] = input[static_cast<std::size_t>(src)];
    }
    for (int i = 0; i < n; ++i) {
        padded[static_cast<std::size_t>(pad + i)] = input[static_cast<std::size_t>(i)];
    }
    for (int i = 0; i < pad; ++i) {
        const int src = std::max(0, n - 2 - i);
        padded[static_cast<std::size_t>(pad + n + i)] = input[static_cast<std::size_t>(src)];
    }

    std::vector<float> output(static_cast<std::size_t>(n), 0.0f);
    for (int i = 0; i < n; ++i) {
        double acc = 0.0;
        for (int j = 0; j < k; ++j) {
            acc += static_cast<double>(padded[static_cast<std::size_t>(i + j)]) * kernel[static_cast<std::size_t>(j)];
        }
        output[static_cast<std::size_t>(i)] = static_cast<float>(acc);
    }
    return output;
}

}  // namespace

Preprocessor::Preprocessor(const ModelMetadata& metadata)
    : metadata_(metadata), config_(metadata.preprocessing) {
    if (metadata_.n_chans <= 0 || metadata_.n_times <= 0) {
        throw std::runtime_error("Preprocessor requires positive n_chans and n_times");
    }
    if (config_.filter_method != "fir") {
        throw std::runtime_error("Only FIR preprocessing is supported");
    }
    if (config_.filter_phase != "zero") {
        throw std::runtime_error("Only zero-phase FIR is supported");
    }
    design_fir_bandpass();
}

void Preprocessor::design_fir_bandpass() {
    fir_coeffs_ = design_firwin_bandpass(
        metadata_.sfreq,
        config_.low_cut_hz,
        config_.high_cut_hz);
}

void Preprocessor::scale_inplace(std::vector<float>& window_nct) const {
    if (config_.multiply_factor == 1.0f) {
        return;
    }
    for (float& value : window_nct) {
        value *= config_.multiply_factor;
    }
}

void Preprocessor::filter_channel_inplace(std::vector<float>& channel) const {
    // Zero-phase: forward then reverse (filtfilt-style), reflection-padded convolution.
    auto forward = convolve_same(channel, fir_coeffs_);
    std::reverse(forward.begin(), forward.end());
    auto backward = convolve_same(forward, fir_coeffs_);
    std::reverse(backward.begin(), backward.end());
    channel = std::move(backward);
}

void Preprocessor::ems_channel_inplace(std::vector<float>& channel) const {
    // braindecode exponential_moving_standardize (per channel).
    const std::size_t n_times = channel.size();
    if (n_times == 0) {
        return;
    }

    const double alpha = static_cast<double>(config_.standardize_factor_new);
    if (!(alpha > 0.0 && alpha <= 1.0)) {
        throw std::runtime_error("standardize_factor_new must be in (0, 1]");
    }
    const double inv_alpha = 1.0 - alpha;
    const double eps = static_cast<double>(config_.standardize_eps);

    std::vector<double> data(n_times);
    for (std::size_t i = 0; i < n_times; ++i) {
        data[i] = static_cast<double>(channel[i]);
    }

    std::vector<double> ones(n_times, 1.0);
    const auto d = lfilter_forward({1.0}, inv_alpha, ones);
    const auto n = lfilter_forward({1.0}, inv_alpha, data);

    std::vector<double> meaned(n_times);
    std::vector<double> demeaned(n_times);
    for (std::size_t i = 0; i < n_times; ++i) {
        meaned[i] = n[i] / d[i];
        demeaned[i] = data[i] - meaned[i];
    }

    std::vector<double> squared(n_times);
    for (std::size_t i = 0; i < n_times; ++i) {
        squared[i] = demeaned[i] * demeaned[i];
    }
    const auto n_sq = lfilter_forward({1.0}, inv_alpha, squared);

    for (std::size_t i = 0; i < n_times; ++i) {
        const double var = n_sq[i] / d[i];
        const double denom = std::max(eps, std::sqrt(var));
        channel[i] = static_cast<float>(demeaned[i] / denom);
    }

    const int init = config_.standardize_init_block_size;
    if (init > 0) {
        const std::size_t block = std::min(static_cast<std::size_t>(init), n_times);
        double sum = 0.0;
        for (std::size_t i = 0; i < block; ++i) {
            sum += data[i];
        }
        const double mean = sum / static_cast<double>(block);
        double var_sum = 0.0;
        for (std::size_t i = 0; i < block; ++i) {
            const double delta = data[i] - mean;
            var_sum += delta * delta;
        }
        // numpy.std default is population std (ddof=0).
        const double stddev = std::sqrt(var_sum / static_cast<double>(block));
        const double denom = std::max(eps, stddev);
        for (std::size_t i = 0; i < block; ++i) {
            channel[i] = static_cast<float>((data[i] - mean) / denom);
        }
    }
}

std::vector<float> Preprocessor::apply(const std::vector<float>& window_nct) const {
    const std::size_t expected =
        static_cast<std::size_t>(metadata_.n_chans) *
        static_cast<std::size_t>(metadata_.n_times);

    if (window_nct.size() != expected) {
        throw std::runtime_error("Preprocessor input size does not match metadata");
    }

    std::vector<float> processed = window_nct;
    scale_inplace(processed);

    const int n_chans = metadata_.n_chans;
    const int n_times = metadata_.n_times;
    for (int channel = 0; channel < n_chans; ++channel) {
        std::vector<float> series(static_cast<std::size_t>(n_times));
        const std::size_t base = static_cast<std::size_t>(channel) * n_times;
        for (int t = 0; t < n_times; ++t) {
            series[static_cast<std::size_t>(t)] = processed[base + static_cast<std::size_t>(t)];
        }

        filter_channel_inplace(series);
        ems_channel_inplace(series);

        for (int t = 0; t < n_times; ++t) {
            processed[base + static_cast<std::size_t>(t)] = series[static_cast<std::size_t>(t)];
        }
    }

    return processed;
}

}  // namespace eeg
