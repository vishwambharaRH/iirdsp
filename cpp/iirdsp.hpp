/**
 * @file iirdsp.hpp
 * @brief C++ convenience wrappers for iirdsp library
 *
 * Provides RAII wrapper classes and std::vector helpers for the C API.
 * Optional, desktop-only. Embedded systems use C API directly.
 */

#ifndef IIRDSP_HPP
#define IIRDSP_HPP

#include "iirdsp.h"
#include <vector>
#include <stdexcept>

namespace iirdsp {

/**
 * RAII wrapper for iirdsp_filter_t
 *
 * Automatically initializes/cleans up filter state.
 * Allows RAII semantics in C++.
 */
class Filter {
public:
    /**
     * Default constructor (empty filter)
     */
    Filter() {
        filter_.num_sections = 0;
    }

    /**
     * Destructor (no-op, but here for clarity)
     */
    ~Filter() = default;

    /**
     * Process a single sample
     */
    iirdsp_real process(iirdsp_real x) {
        return iirdsp_process_sample(&filter_, x);
    }

    /**
     * Process a buffer of samples
     */
    void process_buffer(const iirdsp_real* x, iirdsp_real* y, int N) {
        iirdsp_process_buffer(&filter_, x, y, N);
    }

    /**
     * Process a std::vector
     */
    std::vector<iirdsp_real> process_vector(const std::vector<iirdsp_real>& x) {
        std::vector<iirdsp_real> y(x.size());
        iirdsp_process_buffer(&filter_, x.data(), y.data(), (int)x.size());
        return y;
    }

    /**
     * Zero-phase filtering via filtfilt
     */
    void filtfilt(const iirdsp_real* x, iirdsp_real* y, int N) {
        iirdsp_filtfilt(&filter_, x, y, N);
    }

    /**
     * Zero-phase filtering via filtfilt (std::vector version)
     */
    std::vector<iirdsp_real> filtfilt_vector(const std::vector<iirdsp_real>& x) {
        std::vector<iirdsp_real> y(x.size());
        iirdsp_filtfilt(&filter_, x.data(), y.data(), (int)x.size());
        return y;
    }

    /**
     * Reset filter state
     */
    void reset() {
        iirdsp_filter_reset(&filter_);
    }

    /**
     * Access underlying C structure
     */
    iirdsp_filter_t* c_filter() { return &filter_; }
    const iirdsp_filter_t* c_filter() const { return &filter_; }

protected:
    iirdsp_filter_t filter_;
};

/**
 * Butterworth low-pass filter
 */
class ButterLowPass : public Filter {
public:
    /**
     * Initialize low-pass filter
     *
     * @param order Filter order
     * @param cutoff_hz Cutoff frequency (Hz)
     * @param fs_hz Sampling frequency (Hz)
     */
    ButterLowPass(int order, iirdsp_real cutoff_hz, iirdsp_real fs_hz) {
        if (butter_lowpass_init(&filter_, order, cutoff_hz, fs_hz) != 0) {
            throw std::runtime_error("Failed to initialize low-pass filter");
        }
    }
};

/**
 * Butterworth high-pass filter
 */
class ButterHighPass : public Filter {
public:
    /**
     * Initialize high-pass filter
     *
     * @param order Filter order
     * @param cutoff_hz Cutoff frequency (Hz)
     * @param fs_hz Sampling frequency (Hz)
     */
    ButterHighPass(int order, iirdsp_real cutoff_hz, iirdsp_real fs_hz) {
        if (butter_highpass_init(&filter_, order, cutoff_hz, fs_hz) != 0) {
            throw std::runtime_error("Failed to initialize high-pass filter");
        }
    }
};

/**
 * Butterworth band-pass filter
 */
class ButterBandPass : public Filter {
public:
    /**
     * Initialize band-pass filter
     *
     * @param order Filter order
     * @param f_low_hz Low cutoff frequency (Hz)
     * @param f_high_hz High cutoff frequency (Hz)
     * @param fs_hz Sampling frequency (Hz)
     */
    ButterBandPass(int order, iirdsp_real f_low_hz, iirdsp_real f_high_hz, iirdsp_real fs_hz) {
        if (butter_bandpass_init(&filter_, order, f_low_hz, f_high_hz, fs_hz) != 0) {
            throw std::runtime_error("Failed to initialize band-pass filter");
        }
    }
};

/**
 * Digital notch filter
 */
class NotchFilter : public Filter {
public:
    /**
     * Initialize notch filter
     *
     * @param f0_hz Notch center frequency (Hz)
     * @param Q Quality factor
     * @param fs_hz Sampling frequency (Hz)
     */
    NotchFilter(iirdsp_real f0_hz, iirdsp_real Q, iirdsp_real fs_hz) {
        if (notch_filter_init(&filter_, f0_hz, Q, fs_hz) != 0) {
            throw std::runtime_error("Failed to initialize notch filter");
        }
    }
};

}  /* namespace iirdsp */

#endif /* IIRDSP_HPP */
