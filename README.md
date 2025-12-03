# Spectro-v3

Real-time spectrum analyzer with waterfall spectrogram and scrollable history buffer.

## Features

- Continuous audio capture with full sample retention
- Waterfall spectrogram display
- Real-time spectrum plot
- Configuration panel
- Scrub through historical buffer

## Dependencies

- **Qt6** (Widgets, Multimedia)
- **FFTW3** (Fast Fourier Transform)
- **Catch2** (testing framework)
- **C++23 compatible compiler** (GCC 13+, Clang 16+)

### Installing on Debian/Ubuntu

```bash
sudo apt update
sudo apt install \
    qt6-base-dev \
    qt6-multimedia-dev \
    libfftw3-dev \
    libcatch2-dev \
    cmake \
    build-essential
```

**Note**: Catch2 v3 may require adding testing repositories or building from source if not available in your distro version. Qt6 requires Debian 12+ or Ubuntu 22.04+.

## Build Instructions

```bash
# Configure
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Run tests
ctest --test-dir build

# Run application (after implementation)
./build/spectro
```

## Project Structure

```
spectro-v3/
├── dsp/              # DSP library (pure C++, no Qt dependencies)
│   ├── include/      # Public DSP headers (.h)
│   ├── src/          # DSP implementation files (.cpp)
│   └── tests/        # DSP unit tests (Catch2)
├── qt6_gui/          # Qt6-based GUI application
│   ├── include/      # GUI component headers (.h)
│   ├── src/          # GUI implementation and main.cpp
│   └── tests/        # GUI component tests (future)
├── docs/             # Architecture and design documentation
└── build/            # Build artifacts (not tracked)
```

The DSP library (`dsp/`) is a standalone component with zero Qt dependencies, depending only on FFTW3 and the C++ standard library. This makes it reusable in non-Qt contexts and allows for alternative GUI implementations in the future.

The Qt6 GUI (`qt6_gui/`) will contain all Qt-specific code including audio capture (Qt Multimedia), visualization widgets, and the main application executable. Future alternative GUIs (e.g., web-based, terminal-based) would reside in separate directories at the same level.

## Development

This project follows Test-Driven Development (TDD) practices. See `TODO.md` for current work items and `docs/architecture.md` for system design.

### Build Targets

- `spectro_dsp` - DSP library (pure C++, no Qt dependencies)
- `spectro_dsp_tests` - DSP unit tests
- `spectro` - Main Qt6 GUI application (future)

### Code Quality

**Linting with clang-tidy:**

```bash
# Run linter on all source files
make lint

# Run linter with automatic fixes (use with caution)
make lint-fix

# Enable clang-tidy during build (slower)
cmake -B build -S . -DENABLE_CLANG_TIDY=ON
```

The project uses clang-tidy with modern C++ best practices configured in `.clang-tidy`. The configuration enforces:
- C++ Core Guidelines
- Modern C++23 idioms
- Performance optimizations
- Readability and naming conventions (m_ prefix for members)
