# Architecture

## Components

- **FFTProcessor** - Computes FFT on audio samples, outputs frequency domain data
- **AudioCapture** - Captures real-time audio from system input device
  - QAudioSource for microphone input
  - Signal emission on new audio chunks
  - Handle buffer overruns gracefully
  - Tests: mock audio input, signal emission, error handling
- **AudioBuffer** - Unbounded multi-channel audio storage with efficient append
  - Thread-safe vector with reserve strategy
  - Efficient random access for scrubbing
  - Memory growth strategy for long recordings
  - Tests: append performance, memory usage, thread safety
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

## Architecture Patterns 
- Qt signal/slot for decoupled communication
- Abstract interfaces for components
- Dependency injection for testability
- Mock objects for external dependencies
- Performance benchmarks to guide design

## Conventions
- Doxygen comments for public API, Markdown docs (`docs/`) for architecture
- use m_ prefix for member variables