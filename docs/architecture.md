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

- **`Settings`**: Application configuration (QObject)
  - Single source of truth for all settings
  - FFT settings: transform size, window type, window stride
  - Display settings: aperture (floor/ceiling dB), colormap
  - Emits signals when settings change
  - Validates settings in setters (e.g., stride > 0)
  - Prevents redundant updates (only emit if value changed)
  - Future: Serialization for save/load configuration

### Controllers (Coordination & Logic)

- **`SpectrogramController`**: Coordinates data flow and view state
  - Owns `STFTProcessor`, `FFTProcessor`, `FFTWindow` per channel; recreates on settings change
  - Observes `AudioBuffer.samplesAdded()` → triggers view update for new rows
  - Observes `Settings` signals → recreates DSP objects or updates internal state
  - Tracks (in sample positions):
    - `live_mode`: whether view follows live edge
    - `live_edge_sample`: latest computed sample position
    - `view_bottom_sample`: current view position (always aligned to stride)
  - Mediates data access: `GetRows(channel, start, stride, count)` → calls `STFTProcessor.ComputeSpectrogram()`
  - Tells `SpectrogramView` what sample position to display (view is passive)
  - Receives scroll events from view, manages live/historical mode switching
  - `SetStride(stride)` → snaps `view_bottom_sample` to new alignment, updates view
  - `SetFFTSettings()` → recreates `FFTProcessor`, `FFTWindow`, `STFTProcessor`

- **`AudioRecorder`**: Audio capture
  - Uses Qt Multimedia's `QAudioSource`
  - Captures audio samples from microphone/line-in
  - Writes samples to `AudioBuffer`
  - Owns audio settings: input device, sample rate
  - `SetDevice()` → restarts capture with new device

### Views (UI Widgets)

- **`SpectrogramView`**: Waterfall spectrogram display
  - Passive: only paints when told, doesn't observe data signals
  - `SetBottomSample(sample_position)` → stores position, triggers repaint
  - On paint:
    - Derives row count from widget height
    - Queries `Settings` for aperture and colormap
    - Calls `SpectrogramController.GetRows(channel, bottom_sample, stride, row_count)`
    - Renders spectrogram
  - Forwards scroll events (as sample delta) to controller
  - Listens to `Settings.apertureChanged()`, `Settings.colormapChanged()` → triggers repaint

- **`SpectrumPlot`**: Real-time frequency spectrum line plot
  - Displays most recent (or selected) frequency slice
  - Y-axis: amplitude in dB (configurable range)
  - X-axis: frequency (Hz)
  - Queries `Settings` for aperture
  - Listens to `Settings.apertureChanged()` → triggers repaint

- **`SettingsPanel`**: Settings controls (UI for Settings model)
  - FFT settings: transform size, window stride, window function
  - Display settings: colormap, aperture (floor/ceiling dB)
  - Audio settings: input device selection
  - Initializes UI controls from `Settings` at construction
  - UI changes → calls `Settings` setters (e.g., `Settings.setWindowStride()`)
  - Settings propagate automatically via `Settings` signals to all listeners

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
    Yes → live_edge_sample = sample_position
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
    → Controller.GetRows()  // calls STFTProcessor.ComputeSpectrogram()
    → render spectrogram
```

### Settings Change
```
SettingsPanel UI change → Settings.setFFTSize(new_size)
    ↓
Settings emits fftSettingsChanged(size, type)
    ↓
SpectrogramController receives signal
    → recreates FFTProcessor, FFTWindow, STFTProcessor
    → resets live_edge_sample tracking
    → snaps view_bottom_sample to new stride alignment
    → SpectrogramView.SetBottomSample(view_bottom_sample)
    ↓
SpectrogramView.paint()
    → queries Settings.aperture(), Settings.colormap()
    → Controller.GetRows()  // calls STFTProcessor.ComputeSpectrogram()
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

**Single-threaded**: All components (views, controllers, models, audio capture) run on Qt's main event loop thread.

**Thread Safety**:
- `AudioBuffer` uses mutex for thread-safe read/write
- `STFTProcessor` accesses `AudioBuffer` via `SampleBuffer` (mutex-protected reads)
- Controllers and views run on main thread only

## Component Responsibilities

| Component                  | Responsibilities                                                             |
|----------------------------|------------------------------------------------------------------------------|
| **AudioBuffer**            | Store multi-channel samples (append-only), emit `samplesAdded()` signal      |
| **Settings**               | Own all settings, validate and emit change signals, single source of truth   |
| **SpectrogramController**  | Own DSP objects, mediate data access, coordinate modes                       |
| **SpectrogramView**        | Paint rows at given position                                                 |
| **SpectrumPlot**           | Paint frequency spectrum,                                                    |
| **SettingsPanel**          | UI for Settings model, bidirectional sync                                    |
| **AudioRecorder**          | Capture audio → `AudioBuffer`, own audio settings (device)                   |

## Settings Management

**Settings class is the single source of truth.** All components query Settings and listen to its signals.

| Setting                    | Stored In | Consumed By                       | Recreates DSP? |
|----------------------------|-----------|-----------------------------------|----------------|
| Transform size             | Settings  | SpectrogramController             | Yes            |
| Window function            | Settings  | SpectrogramController             | Yes            |
| Window stride              | Settings  | SpectrogramController             | No             |
| Colormap                   | Settings  | SpectrogramView                   | No             |
| Aperture (floor/ceiling dB)| Settings  | SpectrogramView, SpectrumPlot     | No             |
| Audio input device         | AudioRecorder | AudioRecorder (not in Settings) | No             |

**Settings Signal Flow:**
```
SettingsPanel (UI)
    ├→ Settings.setFFTSize(size)
    ├→ Settings.setWindowType(type)
    ├→ Settings.setWindowStride(stride)
    ├→ Settings.setAperture(min, max)
    └→ Settings.setColormap(type)
        ↓ (Settings emits signals)
    Settings.fftSettingsChanged(size, type)
        → SpectrogramController (recreates DSP objects)
    Settings.windowStrideChanged(stride)
        → SpectrogramController (updates stride, snaps view)
    Settings.apertureChanged(min, max)
        → SpectrogramView, SpectrumPlot (triggers repaint)
    Settings.colormapChanged(type)
        → SpectrogramView (rebuilds LUT, triggers repaint)
```

## Key Design Decisions

### Why MVC?
- **Separation of concerns**: Business logic (models) separate from UI (views)
- **Testability**: Models and controllers can be unit tested without UI
- **Flexibility**: Multiple views can share same data (e.g., Spectrogram and Spectrum use same FFT results)

### Why Settings class as single source of truth?
- **Centralized defaults**: All default values in one place, no scattered constants
- **Automatic propagation**: Change Settings → all listeners notified automatically via signals
- **Trivial testing**: Pass `new Settings()` to components, no mocking required
- **Validation**: Settings setters validate inputs (stride > 0, etc.)
- **Prevents redundant updates**: Setters check if value changed before emitting

### Why on-demand FFT computation (not pre-computed)?
- **Unlimited scrollback**: Audio buffer grows indefinitely; can't pre-compute everything
- **Settings invalidate cache**: Changing FFT size requires recomputation
- **Lazy evaluation**: Only compute what's visible, defer the rest
- **Future-proof**: Architecture supports background pre-computation later

### Why STFTProcessor directly (no caching layer)?
- **YAGNI**: Start simple, add caching only if profiling shows need
- **FFT is fast**: ~1ms for 4096 samples; recomputing visible rows is acceptable
- **Simpler architecture**: Fewer abstractions, easier to understand and debug
- **Future-proof**: Caching can be added to `STFTProcessor` or as a wrapper layer later

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
- Save/load spectrogram images
- Export audio segments
- Multiple channel visualization
- Peak detection and annotation
- Spectrogram measurements (bandwidth, duration)

### Performance
- Profile and potentially migrate `SpectrogramView` to `QOpenGLWidget` with texture uploads
- Add FFT row caching (in `STFTProcessor` or wrapper) if repaint latency becomes an issue
- Consider background thread for pre-computation if scrolling feels sluggish

### Extensibility
- Plugin architecture for custom window functions or color maps
