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
├── src/              # Implementation files (.cpp)
├── include/          # Public headers (.h)
├── tests/            # Catch2 test suites
├── docs/             # Architecture and design documentation
└── build/            # Build artifacts (not tracked)
```

## Development

This project follows Test-Driven Development (TDD) practices. See `TODO.md` for current work items and `docs/architecture.md` for system design.
