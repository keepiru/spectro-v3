# Architecture

## Components

- **FFTProcessor** - Computes FFT on audio samples, outputs frequency domain data
- **AudioCapture** - Captures real-time audio from system input device
- **SampleBuffer** - Stores all captured audio samples for scrubbing/playback (unbounded growth)
- **SpectrogramView** - Qt widget rendering waterfall display
- **SpectrumPlot** - Qt widget rendering real-time frequency spectrum
- **ConfigPanel** - Qt widget for user configuration (FFT size, window function, etc.)
- **MainWindow** - Main application window assembling all components

## Data Flow

Audio → AudioCapture → SampleBuffer → FFTProcessor → Display Widgets

Threading: Qt signal/slot with queued connections for inter-thread communication.
