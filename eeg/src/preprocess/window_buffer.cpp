#include "preprocess/window_buffer.hpp"

#include <stdexcept>

namespace eeg {

WindowBuffer::WindowBuffer(int n_chans, int n_times)
    : n_chans_(n_chans), n_times_(n_times) {
    if (n_chans_ <= 0 || n_times_ <= 0) {
        throw std::runtime_error("WindowBuffer dimensions must be positive");
    }

    rows_.resize(n_times_);
    for (auto& row : rows_) {
        row.resize(n_chans_);
    }
}

void WindowBuffer::push(const std::vector<float>& channels) {
    if (full()) {
        throw std::runtime_error("WindowBuffer::push called on a full buffer");
    }
    if (channels.size() != static_cast<std::size_t>(n_chans_)) {
        throw std::runtime_error("WindowBuffer::push channel count mismatch");
    }
    rows_[write_index_] = channels;
    write_index_++;
}

bool WindowBuffer::full() const {
    return write_index_ >= static_cast<std::size_t>(n_times_);
}

std::vector<float> WindowBuffer::as_nct() const {
    if (!full()) {
        throw std::runtime_error("WindowBuffer::as_nct called on a non-full buffer");
    }
    std::vector<float> nct(static_cast<std::size_t>(n_chans_) * n_times_);
    for (int t = 0; t < n_times_; ++t) {
        for (int c = 0; c < n_chans_; ++c) {
            nct[static_cast<std::size_t>(c) * n_times_ + t] = rows_[t][c];
        }
    }
    return nct;
}

std::vector<float> WindowBuffer::as_nct_from_trial(const std::vector<float>& trial_tc) const {
    if (trial_tc.size() != static_cast<std::size_t>(n_chans_) * n_times_) {
        throw std::runtime_error("WindowBuffer::as_nct_from_trial input size mismatch");
    }
    std::vector<float> nct(trial_tc.size());
    for (int t = 0; t < n_times_; ++t) {
        for (int c = 0; c < n_chans_; ++c) {
            nct[static_cast<std::size_t>(c) * n_times_ + t] = trial_tc[static_cast<std::size_t>(t) * n_chans_ + c];
        }
    }
    return nct;
}

void WindowBuffer::clear() {
    write_index_ = 0;
}

}  // namespace eeg
