# Architecture: Qt6 Spectrum Analyzer

## Overview

Real-time spectrum analyzer with waterfall spectrogram display, built using Qt6 with MVC architecture. The application captures live audio, computes FFTs on-demand, and displays both a scrolling waterfall spectrogram and a real-time frequency spectrum plot. Supports unlimited scrollback through audio history.

## Design Pattern: Model-View-Controller (MVC)

### Models (Data & State)

- **`AudioBuffer`**: Append-only multi-channel audio sample storage
  - Wraps multiple `SampleBuffer` from DSP library
  - Thread-safe for concurrent writes (audio capture) and reads (FFT computation)
  - Emits `samplesAdded(count)` signal when new audio arrives
  - Source of truth for all audio data

- **`FFTCache`**: Computed FFT row cache with on-demand computation
  - Stores computed FFT rows keyed by sample position
  - `GetRows(start_sample, stride, count)` → computes missing rows, returns cached
  - Owns `FFTProcessor` and `FFTWindow` (recreates on settings change)
  - `SetSettings(settings)` → clears cache, emits `settingsChanged()`
  - Thread-safe for future background computation

### Controllers (Coordination & Logic)

- **`SpectrogramController`**: Coordinates data flow and view state
  - Observes `AudioBuffer.samplesAdded()` → triggers FFT computation for new rows
  - Observes `FFTCache.settingsChanged()` → resets tracking, updates view
  - Tracks: `live_mode`, `live_edge_row`, `view_bottom_row`
  - Tells `SpectrogramView` what row to display (view is passive)
  - Receives scroll events from view, manages live/historical mode switching

- **`AudioRecorder`**: Audio capture
  - Uses Qt Multimedia's `QAudioSource`
  - Captures audio samples from microphone/line-in
  - Writes samples to `AudioBuffer`
  - Runs in separate thread managed by Qt

### Views (UI Widgets)

- **`SpectrogramView`**: Waterfall spectrogram display
  - Passive: only paints when told, doesn't observe data signals
  - `SetBottomRow(row_index)` → stores position, triggers repaint
  - On paint: pulls rows from `FFTCache`, renders spectrogram
  - Forwards scroll events to controller
  - Derives row count from widget height

- **`SpectrumPlot`**: Real-time frequency spectrum line plot
  - Displays most recent (or selected) frequency slice
  - Auto-scaling Y-axis (amplitude in dB)
  - X-axis shows frequency (Hz)

- **`ConfigPanel`**: Settings controls
  - Transform size: QSpinBox (power of 2: 256, 512, 1024, 2048, 4096)
  - Window stride: QSpinBox (hop size in samples)
  - Window function: QComboBox (Rectangular, Hann)
  - Calls `FFTCache.SetSettings()` on user input

- **`MainWindow`**: Top-level application window
  - QSplitter layout: left (views), right (config panel)
  - Left side: QVBoxLayout with `SpectrogramView` (top) and `SpectrumPlot` (bottom)
  - Menu bar: File, View, Help
  - Instantiates and wires all components

## Data Flow

### Live Mode - New Audio Arrives
```
AudioRecorder → AudioBuffer.AddSamples()
    ↓
AudioBuffer emits samplesAdded(count)
    ↓
SpectrogramController receives signal
    ↓
Controller: enough samples for new row?
    Yes → FFTCache.GetRow(position)  // cache it
        → view_bottom_row = live_edge_row
        → SpectrogramView.SetBottomRow(view_bottom_row)
    ↓
SpectrogramView.paint()
    → FFTCache.GetRows(bottom_row, stride, row_count)
    → render spectrogram
```

### Historical Mode - User Scrolls Back
```
User scrolls → SpectrogramView forwards to Controller
    ↓
Controller: live_mode = false
    → view_bottom_row = scroll_position
    → SpectrogramView.SetBottomRow(view_bottom_row)
    ↓
SpectrogramView.paint()
    → FFTCache.GetRows()  // computes missing rows on-demand
    → render spectrogram
```

### Settings Change
```
ConfigPanel → FFTCache.SetSettings(new_settings)
    ↓
FFTCache: clears cache, recreates FFTProcessor/FFTWindow
    → emits settingsChanged()
    ↓
SpectrogramController receives signal
    → resets live_edge tracking
    → SpectrogramView.SetBottomRow(current_position)
    ↓
SpectrogramView.paint()
    → FFTCache.GetRows()  // recomputes visible rows
    → render spectrogram
```

### Jump to Live
```
User clicks "Live" button → Controller
    ↓
Controller: live_mode = true
    → view_bottom_row = live_edge_row
    → SpectrogramView.SetBottomRow(view_bottom_row)
    ↓
SpectrogramView.paint()
    → render latest data
```

## Threading Model

1. **Qt Main Thread**: All views, controllers, models, event loop
2. **Audio Capture Thread**: Qt Multimedia manages `QAudioSource` internally

**Thread Safety**:
- `AudioBuffer` uses mutex for thread-safe read/write
- `FFTCache` uses mutex (prepared for future background computation)
- Controllers and views run on main thread only

## Component Responsibilities

| Component | Responsibilities |
|-----------|-----------------|
| **AudioBuffer** | Store samples (append-only), emit `samplesAdded()` signal |
| **FFTCache** | Compute & cache FFT rows on-demand, own DSP objects, manage settings |
| **SpectrogramController** | Coordinate live/historical mode, trigger computation, notify view |
| **SpectrogramView** | Paint rows at given position, forward scroll events to controller |
| **ConfigPanel** | UI for settings → `FFTCache.SetSettings()` |
| **AudioRecorder** | Capture audio → `AudioBuffer` |

## Key Design Decisions

### Why MVC?
- **Separation of concerns**: Business logic (models) separate from UI (views)
- **Testability**: Models and controllers can be unit tested without UI
- **Flexibility**: Multiple views can observe same model (e.g., Spectrogram and Spectrum using FFTCache)

### Why on-demand FFT computation (not pre-computed)?
- **Unlimited scrollback**: Audio buffer grows indefinitely; can't pre-compute everything
- **Settings invalidate cache**: Changing FFT size requires recomputation
- **Lazy evaluation**: Only compute what's visible, defer the rest
- **Future-proof**: Architecture supports background pre-computation later

### Why FFTCache instead of STFTProcessor?
- `STFTProcessor` computes batches; we need incremental/on-demand rows
- Cache manages its own `FFTProcessor` and `FFTWindow` instances
- Row = single FFT at a sample position; spectrogram = collection of rows

### Why passive SpectrogramView?
- **Single responsibility**: View only renders, doesn't manage state
- **Controller owns coordination**: Live mode, scroll position, timing
- **Testability**: Controller logic can be unit tested without UI
- **Simplicity**: View doesn't observe multiple signals, just paints when told

### Why no timer?
- **Event-driven**: Audio arrival triggers computation, not arbitrary timer
- **Efficient**: Only compute when new data available
- **Natural pacing**: Display updates at rate of incoming audio

### Why controller manages scroll state?
- **Centralized state**: Live mode and scroll position in one place
- **Mode switching**: Controller decides when to exit live mode
- **Future features**: Scrubbing, bookmarks, etc. all go through controller

## Future Architecture Considerations

### Backlog
- [ ] Background thread for pre-computation (ahead of scroll, historical regions)
- [ ] Cache eviction policy (LRU or distance-from-view) for memory management

### Performance
- Profile and potentially migrate `SpectrogramView` to `QOpenGLWidget` with texture uploads
- Consider lock-free data structures for `FFTCache` if contention becomes an issue

### Extensibility
- Plugin architecture for custom window functions or color maps
