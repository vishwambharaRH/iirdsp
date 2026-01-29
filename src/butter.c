/**
 * @file butter.c
 * @brief Butterworth IIR filter design implementation
 *
 * All filters are designed using the classical digital IIR design pipeline:
 *   1. Analog Butterworth prototype (s-domain)
 *   2. Frequency transformation (for high-pass and band-pass)
 *   3. Bilinear transform with pre-warping
 *   4. Pole/zero pairing into second-order sections
 *   5. Direct Form II Transposed coefficients
 */

#include "butter.h"
#include <math.h>
#include <string.h>

/* Mathematical constants */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * Compute Butterworth analog prototype poles
 *
 * For a Butterworth filter of order N, the poles in the s-plane are:
 *   p_k = e^(j * pi * (2*k + N + 1) / (2*N))
 *   for k = 0, 1, ..., N-1
 *
 * We store only the left-half plane (stable) poles as (real, imag) pairs.
 *
 * @param order Filter order N
 * @param poles Output array of pole pairs (2*order real values: [re0, im0, re1, im1, ...])
 */
static void butter_analog_poles(int order, iirdsp_real* poles)
{
    for (int k = 0; k < order; k++) {
        iirdsp_real angle = M_PI * (2.0 * k + order + 1.0) / (2.0 * order);
        poles[2*k]     = -cos(angle);  /* Real part */
        poles[2*k + 1] =  sin(angle);  /* Imaginary part */
    }
}

/**
 * Bilinear transform with pre-warping
 *
 * Pre-warping formula for a given frequency wc_analog:
 *   wc_digital = 2 * fs * tan(wc_digital / (2 * fs))
 *
 * This ensures that the digital filter has the correct cutoff frequency.
 *
 * For a first-order pole p in the s-plane:
 *   H(s) = 1 / (s - p)
 *
 * After bilinear transform:
 *   z = (1 + s / (2*fs)) / (1 - s / (2*fs))
 *   H(z) has coefficients derived from the pole location.
 *
 * @param poles_s Analog poles (as re, im pairs)
 * @param order Number of poles
 * @param fs_hz Sampling frequency
 * @param wc_hz Normalized cutoff frequency (for pre-warping)
 * @param sos_b Numerator coefficients of SOS (output)
 * @param sos_a Denominator coefficients of SOS (output)
 * @param num_sections Output number of second-order sections
 */
static void bilinear_transform(
    const iirdsp_real* poles_s,
    int order,
    iirdsp_real fs_hz,
    iirdsp_real wc_hz,
    iirdsp_real* sos_b,
    iirdsp_real* sos_a,
    int* num_sections
)
{
    iirdsp_real fs = fs_hz;
    iirdsp_real warp = 2.0 * fs * tan(M_PI * wc_hz / fs);

    *num_sections = (order + 1) / 2;

    /* Pair up poles and create biquads */
    for (int i = 0; i < *num_sections; i++) {
        int pole_idx = 2 * i;
        iirdsp_real p_real = poles_s[pole_idx] * warp;
        iirdsp_real p_imag = (pole_idx + 1 < 2 * order) ? poles_s[pole_idx + 1] * warp : 0.0;

        /* Create second-order section from complex pole pair */
        /* H(z) = (1 + 2*z^-1 + z^-2) / (1 - (2 * Re(p)) * z^-1 + |p|^2 * z^-2) */

        iirdsp_real a0 = 1.0 + 2.0 * p_real + p_real * p_real + p_imag * p_imag;
        
        sos_b[3*i]     = 1.0 / a0;
        sos_b[3*i + 1] = 2.0 / a0;
        sos_b[3*i + 2] = 1.0 / a0;

        sos_a[3*i]     = 1.0;
        sos_a[3*i + 1] = (2.0 * p_real) / (a0 - 1.0);
        sos_a[3*i + 2] = (p_real * p_real + p_imag * p_imag) / (a0 - 1.0);
    }
}

/**
 * Low-pass Butterworth filter initialization
 *
 * @param f Filter structure to initialize
 * @param order Filter order
 * @param cutoff_hz Cutoff frequency (Hz)
 * @param fs_hz Sampling frequency (Hz)
 * @return 0 on success, negative error code on failure
 */
int butter_lowpass_init(
    iirdsp_filter_t* f,
    int order,
    iirdsp_real cutoff_hz,
    iirdsp_real fs_hz
)
{
    if (order <= 0 || order > 2 * IIRDSP_MAX_SECTIONS) {
        return -1;  /* Invalid order */
    }

    /* Compute analog Butterworth prototype poles */
    iirdsp_real poles_s[2 * IIRDSP_MAX_SECTIONS];
    butter_analog_poles(order, poles_s);

    /* Bilinear transform */
    iirdsp_real sos_b[3 * IIRDSP_MAX_SECTIONS];
    iirdsp_real sos_a[3 * IIRDSP_MAX_SECTIONS];
    int num_sections;

    bilinear_transform(poles_s, order, fs_hz, cutoff_hz / fs_hz, sos_b, sos_a, &num_sections);

    /* Populate filter structure */
    f->num_sections = num_sections;
    for (int i = 0; i < num_sections; i++) {
        f->sections[i].b0 = sos_b[3*i];
        f->sections[i].b1 = sos_b[3*i + 1];
        f->sections[i].b2 = sos_b[3*i + 2];
        f->sections[i].a1 = sos_a[3*i + 1];
        f->sections[i].a2 = sos_a[3*i + 2];
        f->sections[i].z1 = 0.0;
        f->sections[i].z2 = 0.0;
    }

    return 0;
}

/**
 * High-pass Butterworth filter initialization
 *
 * High-pass is obtained by transforming the low-pass prototype:
 *   s_lp → wc / s_hp
 *
 * @param f Filter structure to initialize
 * @param order Filter order
 * @param cutoff_hz Cutoff frequency (Hz)
 * @param fs_hz Sampling frequency (Hz)
 * @return 0 on success, negative error code on failure
 */
int butter_highpass_init(
    iirdsp_filter_t* f,
    int order,
    iirdsp_real cutoff_hz,
    iirdsp_real fs_hz
)
{
    if (order <= 0 || order > 2 * IIRDSP_MAX_SECTIONS) {
        return -1;  /* Invalid order */
    }

    /* Compute analog Butterworth prototype poles */
    iirdsp_real poles_s[2 * IIRDSP_MAX_SECTIONS];
    butter_analog_poles(order, poles_s);

    /* Transform poles from low-pass to high-pass: s → wc / s */
    iirdsp_real wc = 2.0 * M_PI * cutoff_hz;
    for (int k = 0; k < order; k++) {
        iirdsp_real p_re = poles_s[2*k];
        iirdsp_real p_im = poles_s[2*k + 1];
        iirdsp_real denom = p_re * p_re + p_im * p_im;
        poles_s[2*k]     = (p_re * wc) / denom;
        poles_s[2*k + 1] = -(p_im * wc) / denom;
    }

    /* Bilinear transform */
    iirdsp_real sos_b[3 * IIRDSP_MAX_SECTIONS];
    iirdsp_real sos_a[3 * IIRDSP_MAX_SECTIONS];
    int num_sections;

    bilinear_transform(poles_s, order, fs_hz, cutoff_hz / fs_hz, sos_b, sos_a, &num_sections);

    /* Populate filter structure */
    f->num_sections = num_sections;
    for (int i = 0; i < num_sections; i++) {
        f->sections[i].b0 = sos_b[3*i];
        f->sections[i].b1 = sos_b[3*i + 1];
        f->sections[i].b2 = sos_b[3*i + 2];
        f->sections[i].a1 = sos_a[3*i + 1];
        f->sections[i].a2 = sos_a[3*i + 2];
        f->sections[i].z1 = 0.0;
        f->sections[i].z2 = 0.0;
    }

    return 0;
}

/**
 * Band-pass Butterworth filter initialization
 *
 * Band-pass is obtained by transforming the low-pass prototype:
 *   s_lp → (s^2 + wc1*wc2) / (s*(wc2 - wc1))
 *
 * This transformation produces 2*order poles (doubles the filter order).
 *
 * @param f Filter structure to initialize
 * @param order Filter order (band-pass will produce 2*order poles)
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
)
{
    if (order <= 0 || 2 * order > 2 * IIRDSP_MAX_SECTIONS) {
        return -1;  /* Invalid order or exceeds max sections */
    }

    /* Compute analog Butterworth prototype poles */
    iirdsp_real poles_s[2 * IIRDSP_MAX_SECTIONS];
    butter_analog_poles(order, poles_s);

    /* Transform poles from low-pass to band-pass */
    iirdsp_real wc1 = 2.0 * M_PI * f_low_hz;
    iirdsp_real wc2 = 2.0 * M_PI * f_high_hz;
    iirdsp_real wc_prod = wc1 * wc2;
    iirdsp_real bw = wc2 - wc1;

    /* Band-pass transformation doubles the number of poles */
    iirdsp_real transformed_poles[2 * IIRDSP_MAX_SECTIONS];
    int transformed_order = 0;

    for (int k = 0; k < order; k++) {
        iirdsp_real p = poles_s[2*k];
        iirdsp_real q = poles_s[2*k + 1];

        /* Solve: s^2 - p*bw*s + wc_prod = 0 */
        iirdsp_real discriminant = (p * bw) * (p * bw) - 4.0 * wc_prod;
        
        if (discriminant >= 0.0) {
            iirdsp_real sqrt_disc = sqrt(discriminant);
            transformed_poles[2 * transformed_order]     = (p * bw + sqrt_disc) / 2.0;
            transformed_poles[2 * transformed_order + 1] = q;
            transformed_order++;

            transformed_poles[2 * transformed_order]     = (p * bw - sqrt_disc) / 2.0;
            transformed_poles[2 * transformed_order + 1] = q;
            transformed_order++;
        }
    }

    /* Bilinear transform */
    iirdsp_real sos_b[3 * IIRDSP_MAX_SECTIONS];
    iirdsp_real sos_a[3 * IIRDSP_MAX_SECTIONS];
    int num_sections;

    bilinear_transform(transformed_poles, transformed_order, fs_hz, 
                      (f_low_hz + f_high_hz) / (2.0 * fs_hz), sos_b, sos_a, &num_sections);

    /* Populate filter structure */
    f->num_sections = num_sections;
    for (int i = 0; i < num_sections; i++) {
        f->sections[i].b0 = sos_b[3*i];
        f->sections[i].b1 = sos_b[3*i + 1];
        f->sections[i].b2 = sos_b[3*i + 2];
        f->sections[i].a1 = sos_a[3*i + 1];
        f->sections[i].a2 = sos_a[3*i + 2];
        f->sections[i].z1 = 0.0;
        f->sections[i].z2 = 0.0;
    }

    return 0;
}
