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

- **`FFTCache`**: Single-channel FFT row cache with on-demand computation
  - One instance per audio channel, owned by `SpectrogramController`
  - Stores computed FFT rows keyed by sample position
  - `GetRows(start_sample, stride, count)` → computes missing rows, returns cached
  - Owns `FFTProcessor` and `FFTWindow`
  - Thread-safe for future background computation

### Controllers (Coordination & Logic)

- **`SpectrogramController`**: Coordinates data flow and view state
  - Owns `FFTCache[]` (one per channel), creates/destroys as settings change
  - Observes `AudioBuffer.samplesAdded()` → triggers FFT computation for new rows
  - Owns `window_stride` setting
  - Tracks (in sample positions):
    - `live_mode`: whether view follows live edge
    - `live_edge_sample`: latest computed sample position
    - `view_bottom_sample`: current view position (always aligned to stride)
  - Mediates data access: `GetRows(channel, start, stride, count)` → delegates to appropriate cache
  - Tells `SpectrogramView` what sample position to display (view is passive)
  - Receives scroll events from view, manages live/historical mode switching
  - `SetStride(stride)` → snaps `view_bottom_sample` to new alignment, updates view

- **`AudioRecorder`**: Audio capture
  - Uses Qt Multimedia's `QAudioSource`
  - Captures audio samples from microphone/line-in
  - Writes samples to `AudioBuffer`
  - Runs in separate thread managed by Qt
  - Owns audio settings: input device, sample rate
  - `SetDevice()` → restarts capture with new device

### Views (UI Widgets)

- **`SpectrogramView`**: Waterfall spectrogram display
  - Passive: only paints when told, doesn't observe data signals
  - `SetBottomSample(sample_position)` → stores position, triggers repaint
  - On paint:
    - Derives row count from widget height
    - Calls `SpectrogramController.GetRows(channel, bottom_sample, stride, row_count)`
    - Renders spectrogram
  - Forwards scroll events (as sample delta) to controller
  - Owns display settings: colormap, aperture (floor/ceiling dB)
  - `SetColormap()`, `SetAperture()` → triggers repaint

- **`SpectrumPlot`**: Real-time frequency spectrum line plot
  - Displays most recent (or selected) frequency slice
  - Y-axis: amplitude in dB (configurable range)
  - X-axis: frequency (Hz)
  - Owns display settings: aperture (floor/ceiling dB)
  - `SetAperture()` → triggers repaint

- **`ConfigPanel`**: Settings controls
  - FFT settings: transform size, window stride, window function
  - Display settings: colormap, aperture (floor/ceiling dB)
  - Audio settings: input device selection
  - Routes settings to appropriate owners:
    - `FFTCache.SetSettings()` for FFT parameters
    - `SpectrogramView.SetColormap()`, `SetAperture()` for display
    - `SpectrumPlot.SetAperture()` for spectrum display
    - `AudioRecorder.SetDevice()` for audio input

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
    Yes → for each channel: cache[channel].GetRow(sample_position)  // cache it
        → live_edge_sample = sample_position
        → if live_mode: view_bottom_sample = live_edge_sample
        → SpectrogramView.SetBottomSample(view_bottom_sample)
    ↓
SpectrogramView.paint()
    → Controller.GetRows(channel, bottom_sample, stride, row_count)
    → render spectrogram
```

### Historical Mode - User Scrolls Back
```
User scrolls → SpectrogramView forwards to Controller
    ↓
Controller: live_mode = false
    → view_bottom_sample = scroll_position (snapped to stride)
    → SpectrogramView.SetBottomSample(view_bottom_sample)
    ↓
SpectrogramView.paint()
    → Controller.GetRows()  // delegates to cache, computes missing on-demand
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
    → resets live_edge_sample tracking
    → snaps view_bottom_sample to new stride alignment
    → SpectrogramView.SetBottomSample(view_bottom_sample)
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
    → view_bottom_sample = live_edge_sample
    → SpectrogramView.SetBottomSample(view_bottom_sample)
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

| Component                  | Responsibilities                                                             |
|----------------------------|------------------------------------------------------------------------------|
| **AudioBuffer**            | Store multi-channel samples (append-only), emit `samplesAdded()` signal      |
| **FFTCache**               | Compute & cache FFT rows for single channel, own DSP objects                 |
| **SpectrogramController**  | Own FFTCache[] per channel, mediate data access, coordinate modes, own stride|
| **SpectrogramView**        | Paint rows at given position, own display settings (colormap, aperture)      |
| **SpectrumPlot**           | Paint frequency spectrum, own display settings (aperture)                    |
| **ConfigPanel**            | UI for all settings, route to appropriate owners                             |
| **AudioRecorder**          | Capture audio → `AudioBuffer`, own audio settings (device)                   |

## Settings Distribution

Settings live where they're consumed. `ConfigPanel` routes user input to appropriate owners.

| Setting                    | Owner                             | Invalidates Cache? |
|----------------------------|-----------------------------------|--------------------|
| Transform size             | SpectrogramController             | Yes                |
| Window function            | SpectrogramController             | Yes                |
| Window stride              | SpectrogramController             | No                 |
| Colormap                   | SpectrogramView                   | No                 |
| Aperture (floor/ceiling dB)| SpectrogramView, SpectrumPlot     | No                 |
| Audio input device         | AudioRecorder                     | No                 |

```
ConfigPanel
    ├→ SpectrogramController.SetFFTSettings(transform_size, window_function)  // Recreates caches
    ├→ SpectrogramController.SetStride(stride)
    ├→ SpectrogramView.SetColormap(colormap)
    ├→ SpectrogramView.SetAperture(floor, ceiling)
    ├→ SpectrumPlot.SetAperture(floor, ceiling)
    └→ AudioRecorder.SetDevice(device)
```

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

### Why sample-based positioning (not row indices)?
- **Stable across settings**: Sample position represents "where in the audio" — invariant when stride changes
- **Intuitive UX**: Changing window stride keeps view at same audio position, just different granularity
- **Row index is derived**: `row = sample_position / stride`, computed when needed
- **Alignment**: Controller ensures `view_bottom_sample` is always aligned to current stride (snaps on change)

## Future Architecture Considerations

### Backlog
- [ ] Background thread for pre-computation (ahead of scroll, historical regions)
- [ ] Cache eviction policy (LRU or distance-from-view) for memory management

### Performance
- Profile and potentially migrate `SpectrogramView` to `QOpenGLWidget` with texture uploads
- Consider lock-free data structures for `FFTCache` if contention becomes an issue

### Extensibility
- Plugin architecture for custom window functions or color maps
