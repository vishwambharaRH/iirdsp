/**
 * @file sos.c
 * @brief Second-Order Section (biquad) filtering implementation
 */

#include "sos.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

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
)
{
    for (int n = 0; n < N; n++) {
        y[n] = iirdsp_process_sample(f, x[n]);
    }
}

/**
 * Zero-phase filtering via forward-backward filtering (filtfilt)
 *
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
)
{
    /* Allocate temporary buffer for forward pass */
    iirdsp_real* temp = (iirdsp_real*)malloc(N * sizeof(iirdsp_real));
    if (temp == NULL) {
        return;  /* Out of memory */
    }

    /* Forward pass: x → temp */
    iirdsp_filter_init(f);
    iirdsp_process_buffer(f, x, temp, N);

    /* Reset state */
    iirdsp_filter_init(f);

    /* Reverse temp in-place and filter backward */
    for (int i = 0; i < N / 2; i++) {
        iirdsp_real swap = temp[i];
        temp[i] = temp[N - 1 - i];
        temp[N - 1 - i] = swap;
    }

    iirdsp_process_buffer(f, temp, y, N);

    /* Reverse y in-place */
    for (int i = 0; i < N / 2; i++) {
        iirdsp_real swap = y[i];
        y[i] = y[N - 1 - i];
        y[N - 1 - i] = swap;
    }

    free(temp);
}
