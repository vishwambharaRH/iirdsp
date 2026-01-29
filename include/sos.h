/**
 * @file sos.h
 * @brief Second-Order Section (biquad) filtering
 */

#ifndef IIRDSP_SOS_H
#define IIRDSP_SOS_H

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Biquad (second-order section) state and coefficients
 *
 * Direct Form II Transposed implementation:
 *   y[n] = b0*x[n] + z1
 *   z1 = b1*x[n] - a1*y[n] + z2
 *   z2 = b2*x[n] - a2*y[n]
 */
typedef struct {
    iirdsp_real b0, b1, b2;  /* Numerator coefficients */
    iirdsp_real a1, a2;      /* Denominator coefficients (a0 normalized to 1) */
    iirdsp_real z1, z2;      /* State variables */
} iirdsp_biquad_t;

/**
 * IIR filter as a cascade of second-order sections
 *
 * Properties:
 *   - No dynamic memory allocation
 *   - Fixed, predictable memory footprint
 *   - ISR-safe (no malloc/free in signal path)
 *   - Numerically stable via SOS cascade
 */
typedef struct {
    iirdsp_biquad_t sections[IIRDSP_MAX_SECTIONS];
    int num_sections;
} iirdsp_filter_t;

/**
 * Initialize filter state (zero all state variables)
 *
 * @param f Filter pointer
 */
static inline void iirdsp_filter_init(iirdsp_filter_t* f)
{
    for (int i = 0; i < f->num_sections; i++) {
        f->sections[i].z1 = 0.0;
        f->sections[i].z2 = 0.0;
    }
}

/**
 * Process a single sample through a biquad (Direct Form II Transposed)
 *
 * @param s Biquad pointer
 * @param x Input sample
 * @return Filtered output sample
 */
static inline iirdsp_real iirdsp_biquad_process(iirdsp_biquad_t* s, iirdsp_real x)
{
    iirdsp_real y = s->b0 * x + s->z1;
    s->z1 = s->b1 * x - s->a1 * y + s->z2;
    s->z2 = s->b2 * x - s->a2 * y;
    return y;
}

/**
 * Process a single sample through the entire filter (SOS cascade)
 *
 * @param f Filter pointer
 * @param x Input sample
 * @return Filtered output sample
 */
static inline iirdsp_real iirdsp_process_sample(iirdsp_filter_t* f, iirdsp_real x)
{
    iirdsp_real y = x;
    for (int i = 0; i < f->num_sections; i++) {
        y = iirdsp_biquad_process(&f->sections[i], y);
    }
    return y;
}

/**
 * Reset filter state (zero all state variables)
 *
 * @param f Filter pointer
 */
static inline void iirdsp_filter_reset(iirdsp_filter_t* f)
{
    iirdsp_filter_init(f);
}

/**
 * Process a buffer of samples through the filter
 *
 * @param f Filter pointer
 * @param x Input signal (length N)
 * @param y Output signal (length N)
 * @param N Number of samples
 */
void iirdsp_process_buffer(
    iirdsp_filter_t* f,
    const iirdsp_real* x,
    iirdsp_real* y,
    int N
);

/**
 * Zero-phase filtering via forward-backward filtering (filtfilt)
 *
 * Offline-only. Requires entire buffer in memory.
 * Algorithm:
 *   1. Forward filter x → temp
 *   2. Reset state
 *   3. Reverse temp
 *   4. Filter reversed → y
 *   5. Reverse y in-place
 *
 * @param f Filter pointer
 * @param x Input signal (length N)
 * @param y Output signal (length N), can alias x
 * @param N Number of samples
 */
void iirdsp_filtfilt(
    iirdsp_filter_t* f,
    const iirdsp_real* x,
    iirdsp_real* y,
    int N
);

#ifdef __cplusplus
}
#endif

#endif /* IIRDSP_SOS_H */
