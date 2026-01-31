# Architecture: Qt6 Spectrum Analyzer

## Overview

Real-time spectrum analyzer with waterfall spectrogram display, built using Qt6 with MVC architecture. The application captures live audio, computes FFTs on-demand, and displays both a scrolling waterfall spectrogram and a real-time frequency spectrum plot. Supports unlimited scrollback through audio history.

## Design Pattern: Model-View-Controller (MVC)

### Models (Data & State)

- **`AudioBuffer`**: Append-only multi-channel audio sample storage
  - Wraps multiple `SampleBuffer` from DSP library
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
  - Observes `FFTSettingsChanged` signal -> recreates DSP objects
  - Provides `GetRows()` method to compute spectrogram data on-demand
  - Implements per-row caching via `mSpectrogramRowCache` to avoid redundant FFT computation
  - Currently view-driven (future: may add live/historical mode tracking)

- **`AudioRecorder`**: Audio capture
  - Uses Qt Multimedia's `QAudioSource`
  - Captures audio samples from microphone/line-in
  - Writes samples to `AudioBuffer`

- **`AudioFile`**: High level audio file orchestration
  - Reads from `IAudioFileReader`
  - Writes samples to `AudioBuffer`
  - Updates progress callback

- **`IAudioFileReader`**: Low level audio file IO
  - Pure virtual interface can be mocked when testing `AudioFile`
  - `AudioFileReader` implementation wraps libsndfile

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
  - Listens to `Settings.apertureChanged()` -> triggers repaint

- **`ScaleView`**: Horizontal frequency scale
  - Sits between `SpectrogramView` and `SpectrumPlot`
  - Provides ticks and frequency labels

- **`SettingsPanel`**: Settings controls (UI for Settings model)
  - FFT settings: transform size, window stride, window function
  - Display settings: colormap, aperture (floor/ceiling dB)
  - Audio settings: input device selection
  - Initializes UI controls from `Settings` at construction
  - UI changes -> calls `Settings` setters (e.g., `Settings.setWindowStride()`)
  - Settings propagate automatically via `Settings` signals to all listeners

- **`MainWindow`**: Top-level application window
  - QSplitter layout: left (views), right (config panel)
  - Left side: QVBoxLayout with `SpectrogramView` (top) and `SpectrumPlot` (bottom)
  - Menu bar: File, View, Help
  - Instantiates and wires all components

## Data Flow

### Live Mode - New Audio Arrives
```
AudioRecorder -> AudioBuffer.AddSamples()
    DataAvailable() Signal
        SpectrogramView.update()
        SpectrumPlot.update()
```

### FFT Settings Change
```
SettingsPanel UI change -> Settings.SetFFTSize,SetWindowScale
    FFTSettingsChanged() signal
        SpectrogramController receives signal
            ResetFFT()
            recreates IFFTProcessor, FFTWindow for each channel
            clears mSpectrogramRowCache
    DisplaySettingsChanged() signal
        Views refresh
```

### Display settings change
```
SettingsPanel UI change -> Settings.SetApertureFloorDecibels,SetApertureCeilingDecibels,SetColorMap, etc
    DisplaySettingsChanged() signal
        Views refresh
```

### Live audio recording
```
AudioRecorder orchestrates process:
    Hardware -> QAudioSource -> QIODevice -> readyRead signal
    -> ReadAudioData() -> AudioBuffer.AddSamples()
    -> DataAvailable() signal -> Views update
```

## FFT cache strategy
- **Current approach**: populate on demand; cache everything until invalidated; never evict.
    - Dead simple
    - Minimizes stutters when seeking through the file.
    - Maximum performance at the price of high memory usage
    - Good tradeoff for now
- **Future improvements**
    - LRU eviction
        - Low complexity
        - Minimal memory usage
        - Handles live-mode perfectly
        - Scrolling performance needs to be measured
        - Useless when seeking through history
    - Windowed eviction
        - Low-medium complexity
        - Minimal memory usage
        - Handles live-mode perfectly
        - Better scrolling through recent history
        - Long seeks will still stutter
    - Look-ahead precache
        - High complexity
        - Significantly improved scrolling
        - Offloads work from UI thread
        - Useful with any eviction strategy


## Settings Management

**Settings class is the single source of truth.** All components query Settings and listen to its signals.

"FFT settings" (transform size, window function) change the transform output.  When changed, the DSP objects are recreated and the FFT cache is cleared, followed by a display refresh.

"Display settings" (stride, colormap, aperture) only change the display output and do not invalidate transforms.

## Key Design Decisions

### MVC
- Separation of concerns: Business logic (models) separate from UI (views)
- Testability: Models and controllers can be unit tested without UI
- Flexibility: Multiple views can share same data (e.g., Spectrogram and Spectrum use same FFT results)

### Settings class is the single source of truth
- Centralized defaults: All default values in one place, no scattered constants
- Automatic propagation: Change Settings -> all listeners notified automatically via signals
- Trivial testing: Pass `Settings()` to components, no mocking required
- Validation: Settings setters validate inputs (stride > 0, etc.)
- Prevents redundant updates: Setters check if value changed before emitting

### view-driven rendering
- View doesn't have to coordinate viewport size with controller
- Controller is stateless, view simply pulls the data it needs
- View's position is encapsulated in the scrollbar
- Live mode lives in `Settings` - requires a little extra plumbing
- Alternative considered: controller manages scrolling state.
    - Simplifies view logic
    - Might make sense when playback is implemented

### Tickless view architecture
- Event-driven: audio arrival triggers computation, not arbitrary timer
- Efficient: Only compute when new data available
- Natural pacing: Display updates at rate of incoming audio

### Scrollbar fully owns view position
- "feels right"
- Requires adhering to QScrollbar's semantics
- Position is effectively a FrameOffset
    - rounding is applied on render, not when position is set
    - This makes it independent from stride settings
    - Only downside: it's an int32_t, which arbitrarily limits data size

### Append-only SampleBuffer and AudioBuffer
- Everything is stored in memory
- Don't have to worry about range invalidation
- Simple, performant zero-copy access, returning `std::span<const float>`

### SpectrogramView renders in main thread
- Simple design
- Rendering code is performance-critical
    - Over 30% of all CPU cycles are spent in the inner loop
- SIMD vectorization helps some
- Many micro-optimizations still possible
- It's fast enough for now, but this may be a bottleneck in the future
- Future improvements:
    - Threading the rendering pipeline
        - Easy 4x performance gain
        - The data access is simple, overhead should be minimal
        - Low complexity
    - QOpenGLWidget
        - Easy way to offload compositing
        - Unsure if this can offload all rendering
        - Extra copying of textures might negate performance gain
        - Medium complexity
    - Raw GL, or other abstraction
        - The rendering bottleneck is gone
        - Replaced with more copy overhead, or complicated caching
        - High complexity

## Future Architecture Considerations

### Backlog
- Save/load spectrogram images
- Export audio segments
- Multiple channel visualization
- Peak detection and annotation
- Spectrogram measurements (bandwidth, duration)

### Performance

#### Future Optimizations
- Batch AudioBuffer.GetSamples() calls to reduce overhead

### Extensibility
- Plugin architecture for custom window functions or color maps
