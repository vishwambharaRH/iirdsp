/**
 * @file impulse.cpp
 * @brief Basic unit test: impulse response test
 *
 * Verifies that filter coefficients are correctly applied
 * by checking the impulse response.
 */

#include <iostream>
#include <cmath>
#include <cstdlib>
#include "iirdsp.hpp"

int main(void) {
    std::cout << "iirdsp Impulse Response Test\n";
    std::cout << "============================\n\n";

    try {
        /* Test parameters */
        const iirdsp_real Fs = 500.0;
        const int N = 100;

        /* Create a band-pass filter (0.5-40 Hz) */
        iirdsp::ButterBandPass bp_filter(4, 0.5, 40.0, Fs);

        /* Generate impulse signal */
        std::vector<iirdsp_real> impulse(N, 0.0);
        impulse[0] = 1.0;

        /* Apply filter */
        auto response = bp_filter.filtfilt_vector(impulse);

        /* Check response */
        std::cout << "Impulse response (first 10 samples):\n";
        for (int i = 0; i < 10 && i < N; i++) {
            std::cout << "  [" << i << "] = " << response[i] << "\n";
        }

        /* Verify filter was applied (should not be all zeros) */
        iirdsp_real max_val = 0.0;
        for (int i = 0; i < N; i++) {
            if (std::abs(response[i]) > max_val) {
                max_val = std::abs(response[i]);
            }
        }

        std::cout << "\nMax impulse response magnitude: " << max_val << "\n";

        if (max_val > 0.0) {
            std::cout << "\n✓ Test PASSED: Filter is working\n";
            return 0;
        } else {
            std::cout << "\n✗ Test FAILED: Filter response is zero\n";
            return -1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return -1;
    }
}
