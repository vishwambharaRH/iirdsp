/**
 * @file notch.h
 * @brief Digital notch filter for narrowband interference (e.g., powerline)
 */

#ifndef IIRDSP_NOTCH_H
#define IIRDSP_NOTCH_H

#include "config.h"
#include "sos.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Design a digital notch filter (second-order IIR)
 *
 * Useful for removing narrowband interference such as:
 *   - 50 Hz / 60 Hz powerline noise
 *   - Other narrowband periodic noise
 *
 * The notch is centered at f0_hz with bandwidth controlled by Q factor.
 * Higher Q = narrower notch.
 *
 * Equivalent algorithm to scipy.signal.iirnotch(f0_hz, Q, fs_hz)
 *
 * @param f Filter structure to initialize (will contain 1 biquad section)
 * @param f0_hz Notch center frequency (Hz)
 * @param Q Quality factor (typically 30-50 for mains noise). Higher Q = narrower notch.
 * @param fs_hz Sampling frequency (Hz)
 * @return 0 on success, negative error code on failure
 */
int notch_filter_init(
    iirdsp_filter_t* f,
    iirdsp_real f0_hz,
    iirdsp_real Q,
    iirdsp_real fs_hz
);

#ifdef __cplusplus
}
#endif

#endif /* IIRDSP_NOTCH_H */
