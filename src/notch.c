/**
 * @file notch.c
 * @brief Digital notch filter implementation
 */

#include "notch.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * Design a digital notch filter (second-order IIR)
 *
 * Standard second-order IIR notch design matching scipy.signal.iirnotch:
 *   - Center at f0_hz
 *   - Bandwidth determined by Q factor
 *   - Direct Form II Transposed implementation
 *
 * @param f Filter structure to initialize (will contain 1 biquad section)
 * @param f0_hz Notch center frequency (Hz)
 * @param Q Quality factor (typically 30-50 for mains noise)
 * @param fs_hz Sampling frequency (Hz)
 * @return 0 on success, negative error code on failure
 */
int notch_filter_init(
    iirdsp_filter_t* f,
    iirdsp_real f0_hz,
    iirdsp_real Q,
    iirdsp_real fs_hz
)
{
    if (Q <= 0.0 || f0_hz <= 0.0 || fs_hz <= 0.0) {
        return -1;  /* Invalid parameters */
    }

    if (f0_hz >= fs_hz / 2.0) {
        return -2;  /* Frequency must be less than Nyquist */
    }

    /* Normalize frequency to [0, pi] */
    iirdsp_real w0 = 2.0 * M_PI * f0_hz / fs_hz;
    
    /* Bandwidth parameter */
    iirdsp_real alpha = sin(w0) / (2.0 * Q);
    iirdsp_real cos_w0 = cos(w0);

    /* Notch filter coefficients (before normalization) */
    iirdsp_real b0 = 1.0;
    iirdsp_real b1 = -2.0 * cos_w0;
    iirdsp_real b2 = 1.0;
    iirdsp_real a0 = 1.0 + alpha;
    iirdsp_real a1 = -2.0 * cos_w0;
    iirdsp_real a2 = 1.0 - alpha;

    /* Normalize coefficients by a0 */
    f->num_sections = 1;
    f->sections[0].b0 = b0 / a0;
    f->sections[0].b1 = b1 / a0;
    f->sections[0].b2 = b2 / a0;
    f->sections[0].a1 = a1 / a0;
    f->sections[0].a2 = a2 / a0;
    f->sections[0].z1 = 0.0;
    f->sections[0].z2 = 0.0;

    return 0;
}