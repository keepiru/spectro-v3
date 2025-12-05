# Architecture: Qt6 Spectrum Analyzer

## Overview

Real-time spectrum analyzer with waterfall spectrogram display, built using Qt6 with MVC architecture. The application captures live audio, computes Short-Time Fourier Transform (STFT) at 60Hz, and displays both a scrolling waterfall spectrogram and a real-time frequency spectrum plot.

## Design Pattern: Model-View-Controller (MVC)

### Models (Data & Business Logic)
- **`AudioBuffer`**: Multi-channel audio storage (wraps multiple `SampleBuffer` instances)
  - Thread-safe for concurrent writes (audio capture) and reads (processing)
  - Emits signals when new data arrives
  - Inherits `QObject` for Qt signal/slot integration

- **`SpectrogramModel`**: Spectrogram settings and computed data
  - Settings: transform size, window stride, window function type
  - Data: 2D matrix of frequency magnitudes over time
  - Emits `settingsChanged()` and `dataChanged()` signals
  - Inherits `QObject`

### Controllers (Orchestration & Processing)
- **`SpectrogramController`**: Coordinates STFT computation
  - 60Hz QTimer triggers processing loop
  - Reads samples from `AudioBuffer`
  - Uses `STFTProcessor` (from DSP library) to compute spectrogram
  - Updates `SpectrogramModel` with results
  - Owns `FFTProcessor`, `FFTWindow`, `STFTProcessor` instances

- **`AudioRecorder`**: Audio capture thread
  - Uses Qt Multimedia's `QAudioSource`
  - Captures audio samples from microphone/line-in
  - Writes samples to `AudioBuffer`
  - Runs in separate thread managed by Qt

### Views (UI Widgets)
- **`SpectrogramView`**: Waterfall spectrogram display
  - Renders 2D time-frequency representation
  - Scrolls vertically as new data arrives
  - Uses `QPainter` (or `QOpenGLWidget` for performance)
  - Connects to `SpectrogramModel::dataChanged()` signal

- **`SpectrumPlot`**: Real-time frequency spectrum line plot
  - Displays most recent frequency slice
  - Auto-scaling Y-axis (amplitude in dB)
  - X-axis shows frequency (Hz)

- **`ConfigPanel`**: Settings controls
  - Transform size: QSpinBox (power of 2: 256, 512, 1024, 2048, 4096)
  - Window stride: QSpinBox (hop size in samples)
  - Window function: QComboBox (Rectangular, Hann)
  - Modifies `SpectrogramModel` settings

- **`MainWindow`**: Top-level application window
  - QSplitter layout: left (views), right (config panel)
  - Left side: QVBoxLayout with `SpectrogramView` (top) and `SpectrumPlot` (bottom)
  - Menu bar: File, View, Help

## Data Flow

```
Audio Hardware
    ↓
QAudioSource (Qt Multimedia thread)
    ↓
AudioRecorder → writes to → AudioBuffer
    ↑
    | (60Hz timer)
    |
SpectrogramController
    ↓ reads from AudioBuffer
    ↓ computes STFT using STFTProcessor
    ↓ updates
SpectrogramModel
    ↓ emits dataChanged() signal
    ↓
SpectrogramView & SpectrumPlot (Qt main thread)
    ↓ renders
User Display
```

## Threading Model

1. **Qt Main Thread**: All views, models, main window, event loop
2. **Audio Capture Thread**: Qt Multimedia manages `QAudioSource` internally
3. **Controller Timer**: Runs on main thread (60Hz QTimer), triggers STFT computation

**Thread Safety**:
- `AudioBuffer` uses mutex for thread-safe read/write
- Controller reads on timer (main thread), audio capture writes on audio thread
- Models emit signals from main thread only

## Signal/Slot Architecture

**Observer Pattern**: Views observe models, controllers update models.

```
User Action (ConfigPanel)
    → SpectrogramModel::setTransformSize()
    → emit settingsChanged()
    → SpectrogramController receives signal
    → Controller recreates FFTProcessor/FFTWindow with new settings

Controller Timer (60Hz)
    → Controller computes new spectrogram data
    → SpectrogramModel::setSpectrogramData()
    → emit dataChanged()
    → SpectrogramView::onDataChanged() updates display
    → SpectrumPlot::onDataChanged() updates plot
```

## Component Dependencies

```
qt6_gui/
    depends on:
        - dsp/ (SampleBuffer, STFTProcessor, FFTProcessor, FFTWindow)
        - Qt6::Core, Qt6::Widgets, Qt6::Multimedia
```

DSP library remains Qt-independent. Qt6 GUI wraps DSP components with Qt signal/slot integration.

## Key Design Decisions

### 1. Why MVC?
- **Separation of concerns**: Business logic (models) separate from UI (views)
- **Testability**: Models and controllers can be unit tested without UI
- **Flexibility**: Multiple views can observe same model (e.g., multiple spectrograms)

### 2. Why 60Hz update rate?
- Standard display refresh rate
- Balance between responsiveness and CPU usage
- Can be made configurable later

### 3. Why QPainter initially (not QOpenGLWidget)?
- Simpler implementation for MVP
- Sufficient performance for initial testing
- Can migrate to OpenGL if profiling shows bottleneck

### 4. Why direct AudioBuffer write (not queue)?
- `SampleBuffer` is already thread-safe with mutex
- Simpler than queue-based approach
- Lock contention is low (audio writes infrequent, reads are batched)

### 5. Why store spectrogram in model (not just latest slice)?
- Enables historical scrubbing feature
- Supports zoom/pan functionality
- Memory usage is manageable (can implement circular buffer later)

## Future Architecture Considerations

- **Performance**: Profile and potentially migrate `SpectrogramView` to `QOpenGLWidget` with texture uploads
- **Scalability**: Implement circular buffer in `SpectrogramModel` to cap memory usage
- **Extensibility**: Plugin architecture for custom window functions or color maps
- **Multi-channel**: Extend `AudioBuffer` and views to display multiple channels simultaneously
