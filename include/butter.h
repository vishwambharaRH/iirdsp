/**
 * @file butter.h
 * @brief Butterworth IIR filter design and initialization
 */

#ifndef IIRDSP_BUTTER_H
#define IIRDSP_BUTTER_H

#include "config.h"
#include "sos.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Design a Butterworth low-pass filter
 *
 * Computes filter coefficients via:
 *   1. Analog Butterworth prototype (s-domain)
 *   2. Bilinear transform with pre-warping
 *   3. Pole/zero pairing into SOS
 *   4. Direct Form II Transposed form
 *
 * Equivalent to scipy.signal.butter(order, cutoff_hz/fs_hz*2, btype='low', output='sos')
 *
 * @param f Filter structure to initialize
 * @param order Filter order (analog prototype). Max order is IIRDSP_MAX_SECTIONS * 2.
 * @param cutoff_hz Cutoff frequency (Hz)
 * @param fs_hz Sampling frequency (Hz)
 * @return 0 on success, negative error code on failure
 */
int butter_lowpass_init(
    iirdsp_filter_t* f,
    int order,
    iirdsp_real cutoff_hz,
    iirdsp_real fs_hz
);

/**
 * Design a Butterworth high-pass filter
 *
 * Equivalent to scipy.signal.butter(order, cutoff_hz/fs_hz*2, btype='high', output='sos')
 *
 * @param f Filter structure to initialize
 * @param order Filter order (analog prototype). Max order is IIRDSP_MAX_SECTIONS * 2.
 * @param cutoff_hz Cutoff frequency (Hz)
 * @param fs_hz Sampling frequency (Hz)
 * @return 0 on success, negative error code on failure
 */
int butter_highpass_init(
    iirdsp_filter_t* f,
    int order,
    iirdsp_real cutoff_hz,
    iirdsp_real fs_hz
);

/**
 * Design a Butterworth band-pass filter
 *
 * Band-pass transformation produces 2*order poles.
 * Equivalent to scipy.signal.butter(order, [f_low/fs_hz*2, f_high/fs_hz*2], btype='band', output='sos')
 *
 * @param f Filter structure to initialize
 * @param order Filter order (analog prototype). Max order is IIRDSP_MAX_SECTIONS (band-pass produces 2*order poles).
 * @param f_low_hz Low cutoff frequency (Hz)
 * @param f_high_hz High cutoff frequency (Hz)
 * @param fs_hz Sampling frequency (Hz)
 * @return 0 on success, negative error code on failure
 */
int butter_bandpass_init(
    iirdsp_filter_t* f,
    int order,
    iirdsp_real f_low_hz,
    iirdsp_real f_high_hz,
    iirdsp_real fs_hz
);

#ifdef __cplusplus
}
#endif

#endif /* IIRDSP_BUTTER_H */
