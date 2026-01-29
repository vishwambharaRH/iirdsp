# iirdsp — Portable IIR Filter Library for Embedded & Desktop Systems

## Overview

`iirdsp` is a portable C/C++ digital signal processing library implementing a focused subset of classical IIR filters commonly used in biomedical signal processing, with a particular emphasis on ECG preprocessing.

The library provides deterministic, numerically stable implementations of:

- Butterworth low-pass, high-pass, and band-pass filters
- Second-order section (SOS) based IIR filtering
- Zero-phase forward–backward filtering (`filtfilt`)
- Narrowband notch filtering (e.g., powerline interference removal)

The design explicitly targets **both embedded microcontrollers (ESP32, STM32)** and **general-purpose systems (Linux/macOS/Windows)** using a single shared DSP core.

---

## Design Goals

### Primary Goals
- Numerical stability via second-order sections (SOS)
- Deterministic execution suitable for real-time systems
- No dynamic memory allocation in the signal-processing path
- Configurable precision (`float` for MCU, `double` for desktop)
- Binary-compatible C API with optional C++ convenience wrappers

### Non-goals (by design)
- FIR filtering
- FFT-based filtering
- Adaptive filters
- Runtime filter-graph construction
- Zero-latency zero-phase filtering (buffering is required)

---

## Architectural Overview

The library is structured into three layers:

```

┌──────────────────────────────┐
│ Application Code (ECG, etc.) │
├──────────────────────────────┤
│ C++ Convenience Wrappers    │  (optional)
├──────────────────────────────┤
│ Core DSP Engine (C99)        │  ← canonical implementation
└──────────────────────────────┘

````

The **C DSP core** is platform-agnostic and contains all signal-processing logic.  
The **C++ layer** provides RAII, `std::vector` helpers, and desktop ergonomics only.

---

## Filter Design Pipeline

All Butterworth filters follow the classical digital IIR design pipeline:

1. Analog Butterworth prototype (s-domain)
2. Frequency transformation  
   - Low-pass → High-pass  
   - Low-pass → Band-pass
3. Frequency pre-warping
4. Bilinear transform
5. Pole/zero pairing into second-order sections
6. Direct Form II Transposed filtering

This pipeline mirrors the internal behavior of  
`scipy.signal.butter(..., output="sos")`.

---

## Numerical Representation

### Precision Configuration

Precision is configured at compile time:

```c
// config.h
#ifdef IIRDSP_USE_FLOAT
typedef float iirdsp_real;
#else
typedef double iirdsp_real;
#endif
````

* Embedded systems: `float`
* Desktop systems: `double`

All filter coefficients and internal state use `iirdsp_real`.

---

## Core Data Structures

### Biquad (Second-Order Section)

```c
typedef struct {
    iirdsp_real b0, b1, b2;
    iirdsp_real a1, a2;
    iirdsp_real z1, z2;
} iirdsp_biquad_t;
```

### IIR Filter (SOS Cascade)

```c
#define IIRDSP_MAX_SECTIONS 8

typedef struct {
    iirdsp_biquad_t sections[IIRDSP_MAX_SECTIONS];
    int num_sections;
} iirdsp_filter_t;
```

Properties:

* No heap allocation
* Fixed memory footprint
* ISR-safe if required

---

## Filtering Implementation

### Direct Form II Transposed

All filtering uses **Direct Form II Transposed**, chosen for:

* Minimal state
* Improved numerical stability
* Suitability for cascaded SOS

```c
static inline iirdsp_real iirdsp_biquad_process(
    iirdsp_biquad_t* s,
    iirdsp_real x
);
```

Filtering through the SOS cascade:

```c
iirdsp_real iirdsp_process_sample(
    iirdsp_filter_t* f,
    iirdsp_real x
);
```

---

## Zero-Phase Filtering (`filtfilt`)

Zero-phase filtering is implemented via forward–backward filtering and is **offline-only**.

### Algorithm

1. Forward filter input buffer
2. Reset filter state
3. Reverse output buffer
4. Filter again
5. Reverse final output

```c
void iirdsp_filtfilt(
    iirdsp_filter_t* f,
    const iirdsp_real* x,
    iirdsp_real* y,
    int N
);
```

This behavior conceptually matches `scipy.signal.filtfilt`
(edge-padding strategies are intentionally omitted).

---

## Butterworth Filter Design API

### Low-pass

```c
int butter_lowpass_init(
    iirdsp_filter_t* f,
    int order,
    iirdsp_real cutoff_hz,
    iirdsp_real fs_hz
);
```

### High-pass

```c
int butter_highpass_init(
    iirdsp_filter_t* f,
    int order,
    iirdsp_real cutoff_hz,
    iirdsp_real fs_hz
);
```

### Band-pass

```c
int butter_bandpass_init(
    iirdsp_filter_t* f,
    int order,
    iirdsp_real f_low_hz,
    iirdsp_real f_high_hz,
    iirdsp_real fs_hz
);
```

#### Notes

* `order` refers to the analog prototype order
* Band-pass filters produce `2 × order` poles
* Maximum supported order is constrained by `IIRDSP_MAX_SECTIONS`

---

## Notch Filter (Powerline Interference)

A direct digital notch filter is provided for narrowband interference
(e.g., 50/60 Hz mains noise).

```c
int notch_filter_init(
    iirdsp_filter_t* f,
    iirdsp_real f0_hz,
    iirdsp_real Q,
    iirdsp_real fs_hz
);
```

This implementation uses a standard second-order IIR notch formulation
and does not rely on Butterworth prototypes.

---

## Platform Compatibility

### Supported Targets

* Linux
* macOS
* Windows
* ESP32
* STM32 (Cortex-M)

### Platform Independence Rules

* C99 core
* No OS calls
* No file I/O
* No dynamic allocation in filter execution
* No `printf` inside DSP core

---

## Validation Strategy

Numerical correctness is validated against SciPy.

### Reference Implementation

```python
from scipy.signal import butter, sosfilt, filtfilt
```

### Validation Metrics

* Impulse response comparison
* Frequency response magnitude
* Maximum absolute error
* RMS error

Typical expected agreement:

* `double`: ~1e-9
* `float`: ~1e-5

---

## Example: ECG Processing Pipeline

```c
iirdsp_filter_t pqrst;
butter_bandpass_init(&pqrst, 4, 0.5, 40.0, 500.0);

iirdsp_filtfilt(&pqrst, ecg, pqrst_out, N);
```

Equivalent SciPy reference:

```python
b, a = butter(4, [0.5, 40], fs=500, btype="band")
y = filtfilt(b, a, ecg)
```

---

## Build System

The project uses CMake and supports static or shared builds.

```bash
mkdir build && cd build
cmake ..
make
```

---

## Roadmap

* [ ] Core biquad filtering
* [ ] Notch filter
* [ ] Butterworth low-pass
* [ ] Butterworth high-pass
* [ ] Butterworth band-pass
* [ ] filtfilt
* [ ] SciPy parity tests
* [ ] Embedded benchmarks
* [ ] Python bindings (optional)

---

## License

MIT / BSD-style (to be finalized).
```
