/**
 * @file ecg_desktop.c
 * @brief ECG signal preprocessing example for desktop systems
 *
 * Demonstrates:
 *   - Band-pass filtering (0.5 - 40 Hz) for PQRST complex
 *   - Low-pass filtering (0.5 Hz) for baseline drift
 *   - High-pass filtering (40 Hz) for EMG noise
 *   - Notch filtering (50/60 Hz) for powerline interference
 *   - Zero-phase filtering via filtfilt
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "iirdsp.h"

/* Signal processing example for ECG data */
int main(void)
{
    /* System parameters */
    const iirdsp_real Fs = 500.0;  /* Sampling frequency (Hz) */
    const int N_samples = 2500;     /* 5 seconds at 500 Hz */

    /* Allocate signal buffers */
    iirdsp_real* ecg_raw    = (iirdsp_real*)malloc(N_samples * sizeof(iirdsp_real));
    iirdsp_real* pqrst      = (iirdsp_real*)malloc(N_samples * sizeof(iirdsp_real));
    iirdsp_real* baseline   = (iirdsp_real*)malloc(N_samples * sizeof(iirdsp_real));
    iirdsp_real* emg        = (iirdsp_real*)malloc(N_samples * sizeof(iirdsp_real));
    iirdsp_real* powerline  = (iirdsp_real*)malloc(N_samples * sizeof(iirdsp_real));

    if (!ecg_raw || !pqrst || !baseline || !emg || !powerline) {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    /* Generate synthetic ECG signal (for demonstration) */
    for (int n = 0; n < N_samples; n++) {
        iirdsp_real t = n / Fs;
        /* Simulate ECG: 1 Hz base + 5 Hz component + noise */
        ecg_raw[n] = sin(2 * M_PI * 1.0 * t) + 0.5 * sin(2 * M_PI * 5.0 * t) + 0.1 * (rand() / (iirdsp_real)RAND_MAX);
    }

    printf("iirdsp ECG Preprocessing Example\n");
    printf("=================================\n");
    printf("Sampling frequency: %.1f Hz\n", Fs);
    printf("Signal duration: %.1f seconds\n", N_samples / Fs);
    printf("Number of samples: %d\n\n", N_samples);

    /* Initialize filters */
    iirdsp_filter_t pqrst_filter;
    iirdsp_filter_t baseline_filter;
    iirdsp_filter_t emg_filter;
    iirdsp_filter_t notch_filter;

    printf("Initializing filters...\n");

    /* PQRST extraction (0.5 - 40 Hz band-pass) */
    if (butter_bandpass_init(&pqrst_filter, 4, 0.5, 40.0, Fs) != 0) {
        fprintf(stderr, "Failed to initialize PQRST filter\n");
        return -1;
    }
    printf("✓ PQRST filter (0.5-40 Hz, order 4)\n");

    /* Baseline drift (0.5 Hz low-pass) */
    if (butter_lowpass_init(&baseline_filter, 2, 0.5, Fs) != 0) {
        fprintf(stderr, "Failed to initialize baseline filter\n");
        return -1;
    }
    printf("✓ Baseline filter (0.5 Hz, order 2)\n");

    /* EMG noise (40 Hz high-pass) */
    if (butter_highpass_init(&emg_filter, 2, 40.0, Fs) != 0) {
        fprintf(stderr, "Failed to initialize EMG filter\n");
        return -1;
    }
    printf("✓ EMG filter (40 Hz high-pass, order 2)\n");

    /* Powerline interference (50 Hz notch, Q=30) */
    if (notch_filter_init(&notch_filter, 50.0, 30.0, Fs) != 0) {
        fprintf(stderr, "Failed to initialize notch filter\n");
        return -1;
    }
    printf("✓ Notch filter (50 Hz, Q=30)\n\n");

    /* Apply zero-phase filtering (filtfilt) */
    printf("Applying filters...\n");

    iirdsp_filtfilt(&pqrst_filter, ecg_raw, pqrst, N_samples);
    printf("✓ PQRST extraction complete\n");

    iirdsp_filtfilt(&baseline_filter, ecg_raw, baseline, N_samples);
    printf("✓ Baseline extraction complete\n");

    iirdsp_filtfilt(&emg_filter, ecg_raw, emg, N_samples);
    printf("✓ EMG extraction complete\n");

    iirdsp_filtfilt(&notch_filter, ecg_raw, powerline, N_samples);
    printf("✓ Powerline removal complete\n");

    /* Print first 10 samples for verification */
    printf("\nFirst 10 samples (time [s], raw, PQRST):\n");
    for (int n = 0; n < 10; n++) {
        printf("%.3f, %.6f, %.6f\n", n / Fs, ecg_raw[n], pqrst[n]);
    }

    /* Compute RMS of each filtered signal */
    iirdsp_real rms_raw = 0.0, rms_pqrst = 0.0, rms_baseline = 0.0, rms_emg = 0.0;
    for (int n = 0; n < N_samples; n++) {
        rms_raw     += ecg_raw[n] * ecg_raw[n];
        rms_pqrst   += pqrst[n] * pqrst[n];
        rms_baseline += baseline[n] * baseline[n];
        rms_emg     += emg[n] * emg[n];
    }
    rms_raw       = sqrt(rms_raw / N_samples);
    rms_pqrst     = sqrt(rms_pqrst / N_samples);
    rms_baseline  = sqrt(rms_baseline / N_samples);
    rms_emg       = sqrt(rms_emg / N_samples);

    printf("\nSignal RMS values:\n");
    printf("Raw ECG:      %.6f\n", rms_raw);
    printf("PQRST (0.5-40 Hz): %.6f\n", rms_pqrst);
    printf("Baseline (0.5 Hz): %.6f\n", rms_baseline);
    printf("EMG (40+ Hz):  %.6f\n", rms_emg);

    /* Cleanup */
    free(ecg_raw);
    free(pqrst);
    free(baseline);
    free(emg);
    free(powerline);

    printf("\nExample completed successfully!\n");
    return 0;
}
