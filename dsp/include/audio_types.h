#pragma once

/// Audio sample rate in Hz (e.g., 44100, 48000)
/// int is used for compatibility with FFTW3 and libsndfile.
using SampleRate = int;

/// FFT transform size (must be a power of 2)
/// int is used for compatibility with FFTW3.
using FFTSize = int;
