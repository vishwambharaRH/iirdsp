# iirdsp — Initial Code Structure

## Overview

A complete, initial implementation of the **iirdsp** portable IIR filter library for ECG signal processing on embedded and desktop systems.

## Project Structure

```
iirdsp/
├── include/                    # Public C/C++ headers
│   ├── config.h               # Precision configuration & macros
│   ├── sos.h                  # Second-order section (biquad) filtering
│   ├── butter.h               # Butterworth filter design API
│   ├── notch.h                # Notch filter API
│   └── iirdsp.h               # Main umbrella header
├── src/                       # C implementation (DSP core)
│   ├── sos.c                  # Biquad filtering & filtfilt
│   ├── butter.c               # Butterworth filter design
│   └── notch.c                # Notch filter implementation
├── cpp/                       # C++ convenience wrappers
│   └── iirdsp.hpp             # RAII wrapper classes
├── examples/                  # Usage examples
│   └── ecg_desktop.c          # ECG preprocessing demo
├── tests/                     # Test suite
│   └── impulse.cpp            # Impulse response verification
├── CMakeLists.txt             # Build configuration
└── README.md                  # Documentation
```

## What Has Been Implemented

### Core Components

#### 1. **Configuration** (`include/config.h`)
- Compile-time precision selection (`float` vs `double`)
- Maximum filter order configuration
- Platform-agnostic type definitions

#### 2. **Data Structures** (`include/sos.h`)
- `iirdsp_biquad_t`: Second-order section with state
- `iirdsp_filter_t`: SOS cascade (up to 8 sections)
- Direct Form II Transposed implementation

#### 3. **Filter Operations** (`src/sos.c`)
- `iirdsp_process_sample()`: Single-sample processing
- `iirdsp_process_buffer()`: Batch processing
- `iirdsp_filtfilt()`: Zero-phase filtering (forward-backward)

#### 4. **Butterworth Filters** (`src/butter.c`)
- **Low-pass**: `butter_lowpass_init()`
- **High-pass**: `butter_highpass_init()`
- **Band-pass**: `butter_bandpass_init()`

**Design Pipeline:**
1. Analog Butterworth prototype poles (s-domain)
2. Frequency transformation (for HP/BP)
3. Bilinear transform with pre-warping
4. Pole-pairing into second-order sections
5. Direct Form II Transposed coefficients

#### 5. **Notch Filter** (`src/notch.c`)
- `notch_filter_init()`: Center frequency + Q factor
- Typical use: 50/60 Hz powerline noise removal

### C++ Convenience Layer (`cpp/iirdsp.hpp`)
- `iirdsp::Filter` base class with RAII semantics
- `iirdsp::ButterLowPass`, `iirdsp::ButterHighPass`, `iirdsp::ButterBandPass`
- `iirdsp::NotchFilter`
- `std::vector` support for buffer processing

### Examples & Tests

#### **ECG Desktop Example** (`examples/ecg_desktop.c`)
```c
// PQRST extraction (0.5-40 Hz band-pass)
butter_bandpass_init(&pqrst_filter, 4, 0.5, 40.0, 500.0);
iirdsp_filtfilt(&pqrst_filter, ecg, pqrst_out, N);

// Baseline drift (0.5 Hz low-pass)
butter_lowpass_init(&baseline_filter, 2, 0.5, 500.0);

// EMG noise (40 Hz high-pass)
butter_highpass_init(&emg_filter, 2, 40.0, 500.0);

// Powerline (50 Hz notch, Q=30)
notch_filter_init(&notch_filter, 50.0, 30.0, 500.0);
```

#### **Impulse Response Test** (`tests/impulse.cpp`)
- Verifies filter coefficients via C++ wrapper
- Tests vector-based processing
- Runs successfully (✓ PASSED)

## Build System

### CMake Configuration
```bash
mkdir build && cd build
cmake ..
make
```

**Available targets:**
- `iirdsp_core`: Static C library
- `iirdsp`: Header-only C++ wrapper interface
- `ecg_desktop`: ECG example executable
- `test_impulse`: Unit test

**Build options:**
```bash
cmake -DIIRDSP_USE_FLOAT=ON ..  # Use float precision
```

## Key Design Features

✓ **No dynamic memory allocation** in signal processing path  
✓ **Numerically stable** via second-order sections (SOS)  
✓ **Deterministic execution** suitable for real-time systems  
✓ **Binary-compatible C API** with optional C++ wrappers  
✓ **ISR-safe** filtering (no malloc/free in signal path)  
✓ **Configurable precision** (float for MCU, double for desktop)  

## Validation Status

| Component | Status |
|-----------|--------|
| C API (SOS) | ✓ Implemented & tested |
| Butterworth low-pass | ✓ Implemented & tested |
| Butterworth high-pass | ✓ Implemented & tested |
| Butterworth band-pass | ✓ Implemented & tested |
| Notch filter | ✓ Implemented |
| filtfilt (zero-phase) | ✓ Implemented |
| C++ wrappers | ✓ Implemented |
| Example (ECG) | ✓ Working |
| Unit tests | ✓ Passing |

## Next Steps (Roadmap)

- [ ] SciPy numerical validation tests (scipy.signal comparison)
- [ ] Additional filter types (Chebyshev, Elliptic)
- [ ] Python bindings
- [ ] Embedded platform examples (ESP32, STM32)
- [ ] Performance benchmarks
- [ ] Documentation & API reference

## Usage Example

### C API
```c
#include "iirdsp.h"

iirdsp_filter_t filter;
iirdsp_real x[1000], y[1000];

// Design a 4th-order band-pass filter
butter_bandpass_init(&filter, 4, 0.5, 40.0, 500.0);

// Apply zero-phase filtering
iirdsp_filtfilt(&filter, x, y, 1000);
```

### C++ API
```cpp
#include "iirdsp.hpp"

std::vector<double> ecg = ...;

// Create and apply filter in one expression
iirdsp::ButterBandPass filter(4, 0.5, 40.0, 500.0);
auto filtered = filter.filtfilt_vector(ecg);
```

## Compilation Status

**All components compile successfully:**
- ✓ C core (sos.c, butter.c, notch.c)
- ✓ C headers with C++ extern guards
- ✓ C++ wrappers (header-only)
- ✓ Examples and tests

**No warnings or errors** in Apple Clang 17.0
