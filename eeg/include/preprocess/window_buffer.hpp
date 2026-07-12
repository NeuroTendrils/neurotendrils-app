#pragma once

#include <vector>

namespace eeg {

class WindowBuffer {
public:
    /// @param n_chans  Channels per sample.
    /// @param n_times  Samples required to fill one window.
    WindowBuffer(int n_chans, int n_times);

    /// @param channels  One multichannel sample; size must equal n_chans.
    /// @returns  Nothing. Throws if wrong size or buffer already full.
    void push(const std::vector<float>& channels);

    /// @returns  True if n_times samples have been pushed since last clear().
    bool full() const;

    /// @returns  Flat NCT vector, length n_chans * n_times. Throws if not full().
    std::vector<float> as_nct() const;

    /// @param trial_tc  Flat TC trial from test_trials.bin; length n_chans * n_times.
    /// @returns  Flat NCT vector, same length as trial_tc.
    std::vector<float> as_nct_from_trial(const std::vector<float>& trial_tc) const;

    /// @returns  Nothing. Resets write_index_ for the next window.
    void clear();

private:
    int n_chans_ = 0;
    int n_times_ = 0;
    std::size_t write_index_ = 0;
    std::vector<std::vector<float>> rows_;
};

}  // namespace eeg
