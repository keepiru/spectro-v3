# Architecture

## Project Organization

The project is organized into two main directories to create a clean separation between the DSP library and GUI application:

### `dsp/` - DSP Library (Pure C++)
Contains the core digital signal processing components with **zero Qt dependencies**. This library depends only on FFTW3 and the C++ standard library, making it reusable in non-Qt contexts.

**Components:**
- **IFFTProcessor** (`dsp/include/ifft_processor.h`) - Interface for FFT operations enabling dependency injection
- **FFTProcessor** (`dsp/include/fft_processor.h`) - FFTW-based FFT implementation
  - Computes FFT on audio samples, outputs frequency domain data
- **FFTWindow** (`dsp/include/fft_window.h`) - Window functions for spectral leakage reduction
  - Precomputes window coefficients at construction
  - Supported types: Rectangular, Hann
  - Applies windowing to input samples before FFT
- **SampleBuffer** (`dsp/include/sample_buffer.h`) - Thread-safe unbounded audio sample storage
  - Thread-safe vector for mono audio with mutex protection
  - Efficient random access for scrubbing
  - Zero-padding for out-of-bounds access (supports negative indices)
  - Memory growth strategy for long recordings
  - Channel mixing/selection handled at the capture layer
- **STFTProcessor** (`dsp/include/stft_processor.h`) - Short-Time Fourier Transform orchestrator
  - Computes time-frequency spectrograms by sliding FFT windows across audio
  - Orchestrates interaction between SampleBuffer, FFTWindow, and IFFTProcessor

**Testing:** `dsp/tests/` contains Catch2 unit tests for all DSP components, focusing on correctness, thread safety, and edge cases.

**Build Targets:** 
- `spectro_dsp` - DSP library
- `spectro_dsp_tests` - Test executable

### `qt6_gui/` - Qt6 GUI Application
Contains all Qt6-specific code including visualization widgets, audio capture, and the main application executable. This separation allows for potential alternative GUI implementations (web-based, terminal-based, etc.) in the future.

**Components (planned):**
- **AudioCapture** - Captures real-time audio from system input device
  - QAudioSource for microphone input
  - Signal emission on new audio chunks
  - Handle buffer overruns gracefully
  - Tests: mock audio input, signal emission, error handling
- **SpectrogramView** - Waterfall display
  - Scrolling texture/pixmap for history
  - Color mapping (magnitude to RGB)
  - Time axis with scrubbing capability
  - Efficient rendering (OpenGL or QImage)
- **SpectrumPlot** - Real-time frequency display
  - QCustomPlot or QPainter-based widget
  - Logarithmic frequency axis
  - dB scale for magnitudes
  - Update at ~60 FPS without blocking
- **ConfigPanel** - User controls
  - FFT size selection (dropdown: 512, 1024, 2048, 4096, 8192)
  - Window function selection
  - Color scheme for spectrogram
  - Audio device selection
- **MainWindow** - Main application window assembling all components
  - Layout: Spectrogram top, Spectrum bottom, Config right
  - Menu bar (File, View, Help)
  - Status bar (sample rate, buffer size, CPU usage)
- **main.cpp** - Application entry point

**Testing:** `qt6_gui/tests/` will contain tests for GUI components (future)

**Build Targets (future):** 
- `qt6_gui_lib` - GUI component library
- `spectro` - Main executable

## Architecture Patterns 
- Qt signal/slot for decoupled communication
- Abstract interfaces for components
- Dependency injection for testability
- Mock objects for external dependencies
- Performance benchmarks to guide design

## Conventions
- Doxygen comments for public API, Markdown docs (`docs/`) for architecture
- use m_ prefix for member variables