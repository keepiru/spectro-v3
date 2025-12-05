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

**Architecture:**
- **Signal-based communication:** All components communicate via Qt signals/slots for loose coupling
- **Asynchronous updates:** ConfigPanel emits signals immediately; widgets read values on next render (~60 FPS)
- **QThread workers:** Audio capture and STFT computation run on separate threads to avoid blocking GUI
- **Live parameter updates:** FFT size, window function, colormap, and display scaling apply on next render cycle
- **QFormLayout controls:** Professional label-beside-control appearance in ConfigPanel

**Components:**
- **MainWindow** (`qt6_gui/include/main_window.h`)
  - QHBoxLayout: left VBox (spectrogram + spectrum) + right ConfigPanel (~300px fixed width)
  - Spectrogram:Spectrum stretch ratio 7:3
  - Menu bar: File (save/export/quit), View (zoom/fullscreen), Help (about/shortcuts)
  - Status bar: sample rate, buffer size, CPU usage, playback position
  - Integration point: connects all component signals/slots

- **ConfigPanel** (`qt6_gui/include/config_panel.h`)
  - QFormLayout with labeled controls
  - FFT size: QComboBox (512/1024/2048/4096/8192, default 1024)
  - Window function: QComboBox (Rectangular/Hann, default Hann)
  - Zoom: changes overlap to zoom vertically
  - Colormap: QComboBox (Grayscale/Jet/Viridis/Hot, default Viridis)
  - Floor/Ceiling: QDoubleSpinBox (-120 to 0 dB / -60 to 20 dB, defaults -80/0)
  - Audio device: QComboBox + refresh button (QMediaDevices::audioInputs())
  - Start/Stop capture: QPushButton (disabled when no device)
  - Signals: fftSizeChanged, windowTypeChanged, overlapPercentChanged, colormapChanged, floorChanged, ceilingChanged, captureStartRequested, captureStopRequested

- **SpectrogramView** (`qt6_gui/include/spectrogram_view.h`)
  - QWidget with QImage-based rendering
  - Scrolling waterfall display (time on Y-axis, frequency on X-axis)
  - ColormapConverter integration for magnitude-to-RGB mapping per channel
  - Mouse scrubbing: click/drag to navigate history
  - Mouse scroll to adjust vertical zoom / overlap
  - Time axis labels and grid
  - ~60 FPS update via QTimer
  - Slot: updateSpectrogram(std::vector<std::vector<float>>)

- **SpectrumPlot** (`qt6_gui/include/spectrum_plot.h`)
  - QWidget with QPainter-based rendering
  - Real-time frequency magnitude display
  - Horizontal scale matches SpectrogramView
  - dB scale for magnitudes (20*log10)
  - Axis labels and grid lines
  - ~60 FPS update via QTimer
  - Slot: updateSpectrum(std::vector<float>)

- **AudioCapture** (`qt6_gui/include/audio_capture.h`)
  - QThread-based audio input using QAudioSource (Qt Multimedia)
  - Feeds float samples to multiple SampleBuffers (two of them for stereo)
  - Signal: audioDataReady(QByteArray) for downstream processing
  - Device selection from ConfigPanel
  - Buffer overrun detection and status reporting
  - Graceful error handling (device disconnection)

- **SpectrogramWorker** (`qt6_gui/include/spectrogram_worker.h`)
  - QThread worker for STFTProcessor computation
  - Receives requests to compute spectrogram slices for a window into a SampleBuffer
  - Signal: spectrogramReady(std::vector<std::vector<float>>)
  - Cancellable long-running operations
  - Manages FFTProcessor, FFTWindow instances

- **ColormapConverter** (`qt6_gui/include/colormap_converter.h`)
  - Pure utility class (no Qt base class)
  - enum class Colormap { kGrayscale, kJet, kViridis, kHot }
  - Static method: QRgb MagnitudeToRGB(float magnitude, Colormap scheme, float floor, float ceiling)
  - Perceptually uniform Viridis default
  - Clamps input magnitudes to floor/ceiling range

- **Colormap** (`qt6_gui/include/colormap.h`)
  - Enum definition: enum class Colormap { kGrayscale, kJet, kViridis, kHot }
  - Shared between ConfigPanel and ColormapConverter

- **main.cpp** (`qt6_gui/src/main.cpp`)
  - QApplication initialization
  - MainWindow instantiation and show
  - Event loop execution

**Testing:** `qt6_gui/tests/` uses Qt Test framework (not Catch2) for Qt-specific testing features:
- **test_main_window.cpp:** Widget hierarchy (findChild), menu/status bar presence, layout structure
- **test_config_panel.cpp:** QSignalSpy for all signals, default values, widget ranges, button states
- **test_spectrogram_view.cpp:** QImage rendering tests, mouse event handling, color mapping accuracy
- **test_spectrum_plot.cpp:** Pixel coordinate mapping, dB conversion, axis labels
- **test_audio_capture.cpp:** Signal emission with mock data, SampleBuffer integration
- **test_spectrogram_worker.cpp:** Thread safety, STFTProcessor integration, signal emission
- **test_colormap_converter.cpp:** All colormap schemes, boundary conditions, color accuracy
- **Performance tests:** QBENCHMARK for critical paths, separate from functional tests

**Build Targets:** 
- `qt6_gui_lib` - GUI component library (all widgets and workers)
- `spectro` - Main executable
- `qt6_gui_tests` - Qt Test executable

## Architecture Patterns 
- **Qt signal/slot:** Decoupled communication between components
- **Immediate signal emission:** Signals emit on value change without throttling
- **Asynchronous rendering:** Widgets render at their own pace (~60 FPS), reading current state
- **QThread workers:** Long-running operations (audio capture, STFT) on separate threads
- **Dependency injection:** Components receive dependencies via constructor (e.g., SampleBuffer)
- **QSignalSpy testing:** Test signal emissions without mocks
- **QImage testing:** Programmatic pixel verification for rendering correctness
- **Performance benchmarks:** QBENCHMARK for critical paths, separate from functional tests
- **Mock objects:** For external dependencies
- **Dependency injection:** For testability

## Design Decisions

### Configuration Updates
- **Live updates:** All ConfigPanel parameter changes apply immediately on next render cycle
- **No throttling:** Signals emit on every value change (e.g., spinbox drag); widgets handle frequency

### Layout and Sizing
- **MainWindow layout:** QHBoxLayout with left VBox (visualization) + right ConfigPanel
- **Spectrogram:Spectrum ratio:** 7:3 stretch in VBoxLayout
- **ConfigPanel width:** Fixed ~300px for consistent control sizing
- **QFormLayout:** Label-beside-control for professional appearance and accessibility

### Threading Strategy
- **Audio capture thread:** QThread running QAudioSource to avoid blocking GUI
- **STFT worker thread:** QThread for STFTProcessor computation
- **Main GUI thread:** Rendering only, ~60 FPS via QTimer
- **Signal queuing:** Qt automatically handles cross-thread signals safely

### Audio Device Handling
- **Manual refresh:** Explicit button to re-scan devices (no hot-plug detection)
- **Graceful degradation:** Disabled start button when no device available

### Testing Strategy
- **Qt Test framework:** Use Qt Test (not Catch2) for GUI tests to leverage Qt-specific features
- **QSignalSpy extensively:** Test all signal emissions exhaustively
- **QImage rendering tests:** Render to QImage, verify pixels programmatically
- **No visual inspection:** All tests automated without manual verification
- **Performance tests separate:** QBENCHMARK tests isolated from functional tests
- **Integration over mocking:** Test real widget hierarchy when possible

## Conventions
- **Doxygen comments:** Public API documentation in headers
- **Markdown docs:** Architecture and design decisions in `docs/`
- **Mozilla C++ style guide:** Consistent with DSP library
- **m prefix:** Member variables (enforced by clang-tidy)
- **setObjectName():** Set names on widgets for findChild testing
- **Signal naming:** Use past tense (e.g., `fftSizeChanged` not `fftSizeChange`)