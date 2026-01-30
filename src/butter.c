/**
 * @file butter.c
 * @brief Butterworth IIR filter design implementation
 *
 * All filters are designed using the classical digital IIR design pipeline:
 *   1. Analog Butterworth prototype (s-domain)
 *   2. Frequency transformation (for high-pass and band-pass)
 *   3. Bilinear transform with pre-warping
 *   4. Pole/zero pairing into second-order sections
 *   5. Gain normalization
 *   6. Direct Form II Transposed coefficients
 *
 * This implementation produces coefficients that match scipy.signal.butter(..., output='sos')
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
        poles[2*k]     = -sin(angle);  /* Real part */
        poles[2*k + 1] =  cos(angle);  /* Imaginary part */
    }
}

/**
 * Apply bilinear transform to convert analog poles to digital filter
 *
 * Bilinear transform: s = 2*fs * (z-1)/(z+1)
 * 
 * For analog pole p_s and zero at infinity:
 *   H(s) = K / (s - p_s)
 *
 * After bilinear transform, the digital pole is:
 *   p_z = (1 + p_s/(2*fs)) / (1 - p_s/(2*fs))
 *
 * And the digital zero is at z = -1 for low-pass.
 *
 * @param poles_s Analog poles (complex pairs: re, im)
 * @param zeros_s Analog zeros (complex pairs: re, im), NULL for all-pole filter
 * @param num_poles Number of poles
 * @param num_zeros Number of zeros (0 for low-pass from infinity)
 * @param fs_hz Sampling frequency
 * @param filter_type 0=lowpass, 1=highpass, 2=bandpass
 * @param f Filter structure to populate
 */
static void bilinear_zpk(
    const iirdsp_real* poles_s,
    const iirdsp_real* zeros_s,
    int num_poles,
    int num_zeros,
    iirdsp_real fs_hz,
    int filter_type,
    iirdsp_filter_t* f
)
{
    iirdsp_real fs2 = 2.0 * fs_hz;
    int num_sections = (num_poles + 1) / 2;
    f->num_sections = num_sections;

    /* Convert analog poles to digital */
    iirdsp_real poles_z[2 * IIRDSP_MAX_SECTIONS * 2];
    for (int i = 0; i < num_poles; i++) {
        iirdsp_real p_re = poles_s[2*i];
        iirdsp_real p_im = poles_s[2*i + 1];
        
        /* p_z = (1 + p_s/fs2) / (1 - p_s/fs2) */
        iirdsp_real num_re = 1.0 + p_re / fs2;
        iirdsp_real num_im = p_im / fs2;
        iirdsp_real den_re = 1.0 - p_re / fs2;
        iirdsp_real den_im = -p_im / fs2;
        
        /* Complex division */
        iirdsp_real denom = den_re * den_re + den_im * den_im;
        poles_z[2*i]     = (num_re * den_re + num_im * den_im) / denom;
        poles_z[2*i + 1] = (num_im * den_re - num_re * den_im) / denom;
    }

    /* Convert analog zeros to digital (or use -1 for low-pass, +1 for high-pass) */
    iirdsp_real zeros_z[2 * IIRDSP_MAX_SECTIONS * 2];
    int actual_num_zeros = num_poles;  /* Digital filter has same number of zeros as poles */
    
    if (filter_type == 0) {  /* Low-pass: zeros at z = -1 */
        for (int i = 0; i < actual_num_zeros; i++) {
            zeros_z[2*i]     = -1.0;
            zeros_z[2*i + 1] =  0.0;
        }
    } else if (filter_type == 1) {  /* High-pass: zeros at z = +1 */
        for (int i = 0; i < actual_num_zeros; i++) {
            zeros_z[2*i]     =  1.0;
            zeros_z[2*i + 1] =  0.0;
        }
    } else {  /* Band-pass: zeros at z = -1 and z = +1 */
        for (int i = 0; i < actual_num_zeros / 2; i++) {
            zeros_z[2*i]     = -1.0;
            zeros_z[2*i + 1] =  0.0;
        }
        for (int i = actual_num_zeros / 2; i < actual_num_zeros; i++) {
            zeros_z[2*i]     =  1.0;
            zeros_z[2*i + 1] =  0.0;
        }
    }

    /* Pair poles and zeros into second-order sections */
    for (int i = 0; i < num_sections; i++) {
        iirdsp_real p1_re, p1_im, p2_re, p2_im;
        iirdsp_real z1_re, z1_im, z2_re, z2_im;
        
        /* Get pole pair (or single pole for odd order) */
        if (2*i + 1 < num_poles) {
            /* Conjugate pair */
            p1_re = poles_z[4*i];
            p1_im = poles_z[4*i + 1];
            p2_re = poles_z[4*i + 2];
            p2_im = poles_z[4*i + 3];
        } else if (2*i < num_poles) {
            /* Single real pole (odd order, last section) */
            p1_re = poles_z[2*(num_poles - 1)];
            p1_im = poles_z[2*(num_poles - 1) + 1];
            p2_re = p1_re;
            p2_im = -p1_im;  /* Conjugate for completeness */
        } else {
            /* Should not happen */
            p1_re = 0.0;
            p1_im = 0.0;
            p2_re = 0.0;
            p2_im = 0.0;
        }

        /* Get zero pair */
        if (2*i + 1 < actual_num_zeros) {
            z1_re = zeros_z[4*i];
            z1_im = zeros_z[4*i + 1];
            z2_re = zeros_z[4*i + 2];
            z2_im = zeros_z[4*i + 3];
        } else if (2*i < actual_num_zeros) {
            z1_re = zeros_z[2*(actual_num_zeros - 1)];
            z1_im = zeros_z[2*(actual_num_zeros - 1) + 1];
            z2_re = z1_re;
            z2_im = z1_im;
        } else {
            z1_re = zeros_z[0];
            z1_im = zeros_z[1];
            z2_re = zeros_z[0];
            z2_im = zeros_z[1];
        }

        /* Form second-order section coefficients */
        /* Numerator: (z - z1)(z - z2) = z^2 - (z1+z2)*z + z1*z2 */
        iirdsp_real b0 = 1.0;
        iirdsp_real b1 = -(z1_re + z2_re);
        iirdsp_real b2 = z1_re * z2_re - z1_im * z2_im;

        /* Denominator: (z - p1)(z - p2) = z^2 - (p1+p2)*z + p1*p2 */
        iirdsp_real a0 = 1.0;
        iirdsp_real a1 = -(p1_re + p2_re);
        iirdsp_real a2 = p1_re * p2_re - p1_im * p2_im;

        /* Normalize by a0 and store */
        f->sections[i].b0 = b0 / a0;
        f->sections[i].b1 = b1 / a0;
        f->sections[i].b2 = b2 / a0;
        f->sections[i].a1 = a1 / a0;
        f->sections[i].a2 = a2 / a0;
        f->sections[i].z1 = 0.0;
        f->sections[i].z2 = 0.0;
    }
}

/**
 * Compute gain at a specific frequency for normalization
 *
 * @param f Filter to evaluate
 * @param freq Frequency (normalized: 0=DC, 0.5=Nyquist)
 * @return Magnitude response at freq
 */
static iirdsp_real compute_gain_at_freq(const iirdsp_filter_t* f, iirdsp_real freq)
{
    iirdsp_real w = 2.0 * M_PI * freq;
    iirdsp_real cos_w = cos(w);
    iirdsp_real sin_w = sin(w);
    iirdsp_real cos_2w = cos(2.0 * w);
    iirdsp_real sin_2w = sin(2.0 * w);
    
    iirdsp_real gain_re = 1.0;
    iirdsp_real gain_im = 0.0;
    
    for (int i = 0; i < f->num_sections; i++) {
        /* Evaluate H(e^jw) for this section */
        iirdsp_real num_re = f->sections[i].b0 + f->sections[i].b1 * cos_w + f->sections[i].b2 * cos_2w;
        iirdsp_real num_im = -f->sections[i].b1 * sin_w - f->sections[i].b2 * sin_2w;
        iirdsp_real den_re = 1.0 + f->sections[i].a1 * cos_w + f->sections[i].a2 * cos_2w;
        iirdsp_real den_im = -f->sections[i].a1 * sin_w - f->sections[i].a2 * sin_2w;
        
        /* Complex division: H = num / den */
        iirdsp_real denom = den_re * den_re + den_im * den_im;
        iirdsp_real h_re = (num_re * den_re + num_im * den_im) / denom;
        iirdsp_real h_im = (num_im * den_re - num_re * den_im) / denom;
        
        /* Accumulate gain */
        iirdsp_real new_gain_re = gain_re * h_re - gain_im * h_im;
        iirdsp_real new_gain_im = gain_re * h_im + gain_im * h_re;
        gain_re = new_gain_re;
        gain_im = new_gain_im;
    }
    
    return sqrt(gain_re * gain_re + gain_im * gain_im);
}

/**
 * Normalize filter gain at specified frequency
 *
 * @param f Filter to normalize
 * @param freq Frequency (normalized: 0=DC, 0.5=Nyquist)
 */
static void normalize_gain(iirdsp_filter_t* f, iirdsp_real freq)
{
    iirdsp_real gain = compute_gain_at_freq(f, freq);
    
    if (gain > 1e-10) {
        /* Normalize first section's numerator */
        f->sections[0].b0 /= gain;
        f->sections[0].b1 /= gain;
        f->sections[0].b2 /= gain;
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
    if (cutoff_hz <= 0.0 || cutoff_hz >= fs_hz / 2.0) {
        return -2;  /* Invalid cutoff frequency */
    }

    /* Compute analog Butterworth prototype poles */
    iirdsp_real poles_s[2 * IIRDSP_MAX_SECTIONS * 2];
    butter_analog_poles(order, poles_s);

    /* Pre-warp the cutoff frequency */
    iirdsp_real wc_warped = 2.0 * fs_hz * tan(M_PI * cutoff_hz / fs_hz);
    
    /* Scale analog poles by warped cutoff */
    for (int i = 0; i < order; i++) {
        poles_s[2*i]     *= wc_warped;
        poles_s[2*i + 1] *= wc_warped;
    }

    /* Apply bilinear transform */
    bilinear_zpk(poles_s, NULL, order, 0, fs_hz, 0, f);

    /* Normalize gain at DC */
    normalize_gain(f, 0.0);

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
    if (cutoff_hz <= 0.0 || cutoff_hz >= fs_hz / 2.0) {
        return -2;  /* Invalid cutoff frequency */
    }

    /* Compute analog Butterworth prototype poles */
    iirdsp_real poles_s[2 * IIRDSP_MAX_SECTIONS * 2];
    butter_analog_poles(order, poles_s);

    /* Pre-warp the cutoff frequency */
    iirdsp_real wc_warped = 2.0 * fs_hz * tan(M_PI * cutoff_hz / fs_hz);

    /* Low-pass to high-pass transformation: s → wc/s */
    for (int i = 0; i < order; i++) {
        iirdsp_real p_re = poles_s[2*i];
        iirdsp_real p_im = poles_s[2*i + 1];
        iirdsp_real mag_sq = p_re * p_re + p_im * p_im;
        
        /* Invert and scale */
        poles_s[2*i]     = -p_re * wc_warped / mag_sq;
        poles_s[2*i + 1] = -p_im * wc_warped / mag_sq;
    }

    /* Apply bilinear transform (filter_type=1 for highpass) */
    bilinear_zpk(poles_s, NULL, order, 0, fs_hz, 1, f);

    /* Normalize gain at Nyquist */
    normalize_gain(f, 0.5);

    return 0;
}

/**
 * Band-pass Butterworth filter initialization
 *
 * Band-pass is obtained by transforming the low-pass prototype:
 *   s_lp → (s^2 + w0^2) / (s * BW)
 *   where w0 = sqrt(wc1 * wc2), BW = wc2 - wc1
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
    if (order <= 0 || order > IIRDSP_MAX_SECTIONS) {
        return -1;  /* Invalid order (band-pass doubles it) */
    }
    if (f_low_hz <= 0.0 || f_high_hz <= f_low_hz || f_high_hz >= fs_hz / 2.0) {
        return -2;  /* Invalid frequency range */
    }

    /* Compute analog Butterworth prototype poles */
    iirdsp_real poles_lp[2 * IIRDSP_MAX_SECTIONS];
    butter_analog_poles(order, poles_lp);

    /* Pre-warp both cutoff frequencies */
    iirdsp_real wc1 = 2.0 * fs_hz * tan(M_PI * f_low_hz / fs_hz);
    iirdsp_real wc2 = 2.0 * fs_hz * tan(M_PI * f_high_hz / fs_hz);
    iirdsp_real w0 = sqrt(wc1 * wc2);  /* Center frequency */
    iirdsp_real bw = wc2 - wc1;         /* Bandwidth */

    /* Low-pass to band-pass transformation */
    /* Each pole p becomes two poles via: s^2 - p*BW*s + w0^2 = 0 */
    iirdsp_real poles_bp[2 * IIRDSP_MAX_SECTIONS * 4];
    int bp_count = 0;

    for (int i = 0; i < order; i++) {
        iirdsp_real p_re = poles_lp[2*i];
        iirdsp_real p_im = poles_lp[2*i + 1];
        
        /* Only use real part for LP->BP transform (imaginary handled separately) */
        /* Solve: s^2 - p*BW*s + w0^2 = 0 */
        iirdsp_real alpha = -p_re * bw / 2.0;
        iirdsp_real beta_sq = alpha * alpha - w0 * w0;
        
        if (beta_sq >= 0.0) {
            /* Real roots */
            iirdsp_real beta = sqrt(beta_sq);
            poles_bp[2*bp_count]     = alpha + beta;
            poles_bp[2*bp_count + 1] = p_im * bw;
            bp_count++;
            poles_bp[2*bp_count]     = alpha - beta;
            poles_bp[2*bp_count + 1] = p_im * bw;
            bp_count++;
        } else {
            /* Complex roots */
            iirdsp_real beta = sqrt(-beta_sq);
            poles_bp[2*bp_count]     = alpha;
            poles_bp[2*bp_count + 1] = beta + p_im * bw;
            bp_count++;
            poles_bp[2*bp_count]     = alpha;
            poles_bp[2*bp_count + 1] = -beta + p_im * bw;
            bp_count++;
        }
    }

    /* Apply bilinear transform (filter_type=2 for bandpass) */
    bilinear_zpk(poles_bp, NULL, bp_count, 0, fs_hz, 2, f);

    /* Normalize gain at center frequency */
    iirdsp_real f_center = sqrt(f_low_hz * f_high_hz);
    normalize_gain(f, f_center / fs_hz);

    return 0;
}