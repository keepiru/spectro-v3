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

- **`SpectrogramController`**: Coordinates data flow and FFT computation
  - Owns `IFFTProcessor`, `FFTWindow` per channel; recreates on settings change
  - Observes `FFTSettingsChanged` signal → recreates DSP objects
  - Provides `GetRows()` method to compute spectrogram data on-demand
  - Implements per-row caching via `mSpectrogramRowCache` to avoid redundant FFT computation
  - Currently view-driven (future: may add live/historical mode tracking)

- **`AudioRecorder`**: Audio capture
  - Uses Qt Multimedia's `QAudioSource`
  - Captures audio samples from microphone/line-in
  - Writes samples to `AudioBuffer`

### Views (UI Widgets)

- **`SpectrogramView`**: Waterfall spectrogram display
  - On paint:
    - Derives row count from widget height
    - Queries `Settings` for aperture, colormap, stride, FFT size
  - Future: scroll/scrubbing support, live/historical mode tracking

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
DataAvailable Signal
    ↓
SpectrogramView.update()
SpectrumPlot.update()
```

### Future: Historical Mode - User Scrolls Back
```
[Not yet implemented]
User scrolls → SpectrogramController updates view position
    ↓
Controller: live_mode = false
    → view_bottom_sample = scroll_position (snapped to stride)
    → Triggers SpectrogramView update
    ↓
SpectrogramView.paint()
    → Controller.GetRows() with historical position
    → render spectrogram
```

### Settings Change
```
SettingsPanel UI change → Settings.setFFTSize(new_size)
    ↓
Settings emits FFTSettingsChanged()
    ↓
SpectrogramController receives signal
    → OnFFTSettingsChanged()
    → recreates IFFTProcessor, FFTWindow for each channel
    → clears mSpectrogramRowCache (invalidates all cached rows)
    ↓
Next paint event:
SpectrogramView.paint()
    → queries Settings for FFT size, stride, aperture, colormap
    → Controller.GetRows() recomputes with new settings
    → render spectrogram
```

## Threading Model

**Single-threaded**: All components (views, controllers, models, audio capture) run on Qt's main event loop thread.

**Thread Safety**:
- `AudioBuffer` uses mutex for thread-safe read/write
- `SpectrogramController` accesses `AudioBuffer` via `SampleBuffer` (mutex-protected reads)
- `mSpectrogramRowCache` is accessed only from main thread (no locking needed)
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
    ├→ Settings.SetFFTSize(size)
    ├→ Settings.SetWindowType(type)
    ├→ Settings.SetWindowStride(stride)
    ├→ Settings.SetApertureMinDecibels(min)
    ├→ Settings.SetApertureMaxDecibels(max)
    └→ Settings.SetColorMap(type)
        ↓ (Settings emits signals)
    Settings.FFTSettingsChanged()
        → SpectrogramController (recreates DSP objects, clears cache)
    Settings.ApertureChanged()
        → Views (triggers repaint with new dB range)
    Settings.ColorMapChanged()
        → Views (rebuilds LUT, triggers repaint)
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

- **Settings invalidation**: Cache cleared on FFT settings change (size, window type)
- **Future-proof**: Can expand to LRU cache or background pre-computation if needed

### Why view-driven rendering (for now)?
- **Simpler initial implementation**: View calculates position and drives data fetching
- **No state tracking needed**: Controller is stateless except for cache
- **Works for initial use case**: Always displays most recent audio
- **Future enhancement**: Will add live/historical mode with controller-driven positioning

### Why no timer?
- **Event-driven**: Audio arrival triggers computation, not arbitrary timer
- **Efficient**: Only compute when new data available
- **Natural pacing**: Display updates at rate of incoming audio

### Future: Controller-managed scroll state
- **Centralized state**: Live mode and scroll position will move to controller
- **Mode switching**: Controller will decide when to exit live mode
- **Sample-based positioning**: Position by sample (not row) to remain stable across stride changes
- **Alignment**: Controller will snap positions to stride boundaries

## Future Architecture Considerations

### Backlog
- Save/load spectrogram images
- Export audio segments
- Multiple channel visualization
- Peak detection and annotation
- Spectrogram measurements (bandwidth, duration)

### Performance
- Profile and potentially migrate `SpectrogramView` to `QOpenGLWidget` with texture uploads
- Expand per-row cache to LRU cache if memory becomes an issue
- Consider background thread for pre-computation when live/historical mode is added
- Batch AudioBuffer.GetSamples() calls to reduce locking overhead

### Extensibility
- Plugin architecture for custom window functions or color maps
