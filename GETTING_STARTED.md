# Getting Started with iirdsp

## Overview

You now have a complete, working implementation of the **iirdsp** portable IIR filter library for ECG and biomedical signal processing.

## Quick Start (5 minutes)

### 1. Build the project
```bash
cd /Users/vishwam/VSCode/iirdsp
mkdir -p build && cd build
cmake ..
make
```

### 2. Run the example
```bash
./ecg_desktop
```

### 3. Run the test
```bash
./test_impulse
```

---

## File Structure

### Headers (`include/`)
- `config.h` — Precision config, macros, constants
- `sos.h` — Core biquad & SOS filtering API
- `butter.h` — Butterworth filter design API
- `notch.h` — Notch filter API
- `iirdsp.h` — Main umbrella header

### C Implementation (`src/`)
- `sos.c` — Biquad processing, filtfilt
- `butter.c` — Butterworth design (LP/HP/BP)
- `notch.c` — Notch filter design

### C++ Wrappers (`cpp/`)
- `iirdsp.hpp` — RAII classes, std::vector support

### Examples & Tests
- `examples/ecg_desktop.c` — ECG preprocessing demo
- `tests/impulse.cpp` — Impulse response test

---

## Key APIs

### C API (Low-level, ISR-safe)

```c
#include "iirdsp.h"

// Filter design
int butter_lowpass_init(iirdsp_filter_t *f, int order, 
                        iirdsp_real cutoff_hz, iirdsp_real fs_hz);
int butter_highpass_init(iirdsp_filter_t *f, int order,
                         iirdsp_real cutoff_hz, iirdsp_real fs_hz);
int butter_bandpass_init(iirdsp_filter_t *f, int order,
                         iirdsp_real f_low_hz, iirdsp_real f_high_hz,
                         iirdsp_real fs_hz);
int notch_filter_init(iirdsp_filter_t *f, iirdsp_real f0_hz,
                      iirdsp_real Q, iirdsp_real fs_hz);

// Filtering
iirdsp_real iirdsp_process_sample(iirdsp_filter_t *f, iirdsp_real x);
void iirdsp_process_buffer(iirdsp_filter_t *f,
                           const iirdsp_real *x, iirdsp_real *y, int N);
void iirdsp_filtfilt(iirdsp_filter_t *f,
                     const iirdsp_real *x, iirdsp_real *y, int N);
```

### C++ API (High-level, RAII)

```cpp
#include "iirdsp.hpp"

// Band-pass filter
iirdsp::ButterBandPass filter(4, 0.5, 40.0, 500.0);
auto output = filter.filtfilt_vector(input);
```

---

## Example: ECG Processing

```c
#include "iirdsp.h"

iirdsp_filter_t pqrst;
iirdsp_real ecg[2500], output[2500];

// Design 4th-order band-pass (0.5-40 Hz) at 500 Hz
butter_bandpass_init(&pqrst, 4, 0.5, 40.0, 500.0);

// Apply zero-phase filtering
iirdsp_filtfilt(&pqrst, ecg, output, 2500);
```

**Equivalent SciPy code:**
```python
from scipy.signal import butter, filtfilt
b, a = butter(4, [0.5, 40], fs=500, btype='band')
output = filtfilt(b, a, ecg)
```

---

## What Has Been Implemented ✓

- ✓ Core biquad filtering (Direct Form II Transposed)
- ✓ Butterworth low-pass, high-pass, band-pass filters
- ✓ Notch filter for powerline noise
- ✓ Zero-phase filtering (filtfilt)
- ✓ C API with ISR-safe signal processing path
- ✓ C++ RAII wrappers with std::vector support
- ✓ ECG preprocessing example
- ✓ Impulse response unit test
- ✓ CMake build system
- ✓ All code compiles without warnings

---

## Next Steps

1. Read the README for full design documentation
2. Study `examples/ecg_desktop.c` for complete usage
3. Implement embedded version for ESP32/STM32
4. Add SciPy numerical validation tests
5. Profile performance on target hardware

---

**Happy filtering! 🎛️**
