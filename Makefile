# Spectro-v3 -- Real-time spectrum analyzer
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2025-2026 Chris "Kai" Frederick

# Makefile for spectro-v3 routine tasks
# 
# This wraps CMake commands for convenience. The actual build logic
# remains in CMakeLists.txt - this just provides shorter commands.

# Suppress "Entering/Leaving directory" messages
MAKEFLAGS += --no-print-directory

# Configuration
# Detect if running in Docker and use separate build directory
ifeq ($(shell test -f /.dockerenv && echo yes),yes)
    BUILD_DIR := build-docker
    UNITY_DIR := build-docker
else
    BUILD_DIR := build
    UNITY_DIR := build-unity
endif
BUILD_TYPE := Debug
JOBS := $(shell nproc 2>/dev/null || echo 4)

.PHONY: all clean \
		build configure \
		unity configure-unity \
		test test-one \
		lint lint-fix-changed lint-fix lint-files \
        release \
		run bench

# Default target
all: build

# Configure CMake (run once or after CMakeLists.txt changes)
# This is the standard configuration without unity, which is needed for clangd analysis.
configure: ${BUILD_DIR}/compile_commands.json

${BUILD_DIR}/compile_commands.json:
	cmake --preset=default -B ${BUILD_DIR}


# Configure CMake for unity builds (faster builds for development)
configure-unity: ${UNITY_DIR}/compile_commands.json

${UNITY_DIR}/compile_commands.json:
	cmake --preset=unity -B ${UNITY_DIR}

# Build the project (configures if needed)
build: configure
	@echo "Building..."
	cmake --build $(BUILD_DIR) --config $(BUILD_TYPE) -j $(JOBS)

# Build the project with unity
unity: configure-unity
	@echo "Building..."
	cmake --build $(UNITY_DIR) --config $(BUILD_TYPE) -j $(JOBS)

# Clean build artifacts
clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR) $(UNITY_DIR)

# Run tests via CTest
test: build
	@echo "Running tests..."
	ctest --test-dir $(BUILD_DIR) --output-on-failure --progress

# Run single test by name pattern (usage: make test-one NAME=FFTProcessor)
test-one: build
	@echo "Running tests matching '$(NAME)'..."
	ctest --test-dir $(BUILD_DIR) --output-on-failure -R "$(NAME)"

# Release build
release: clean
	@echo "Building release..."
	cmake --preset=release -B ${BUILD_DIR}
	cmake --build $(BUILD_DIR) --config Release -j $(JOBS)

# Lint with clang-tidy
lint: configure
	@echo "Running clang-tidy on source files..."
	find dsp/ qt6_gui/ -name '*.cpp' | xargs run-clang-tidy -p $(BUILD_DIR) -use-color -quiet

# Lint specific files (usage: make lint-file FILE="file1.cpp file2.cpp")
lint-files: configure
	@echo "Running clang-tidy..."
	run-clang-tidy -p $(BUILD_DIR) -use-color -quiet $(filter-out $@,$(MAKECMDGOALS))

# Lint with automatic fixes (use with caution)
lint-fix: configure
	@echo "Running clang-tidy with automatic fixes..."
	find dsp/ qt6_gui/ -name '*.cpp' | xargs run-clang-tidy -p $(BUILD_DIR) -use-color -fix -quiet

# Lint files changed in git
lint-changed: configure
	@echo "Running clang-tidy on changed files..."
	git diff --name-only HEAD | xargs run-clang-tidy -p $(BUILD_DIR) -use-color -quiet

# Lint files changed in git
lint-fix-changed: configure
	@echo "Running clang-tidy on changed files..."
	git diff --name-only HEAD | xargs run-clang-tidy -p $(BUILD_DIR) -use-color -fix -quiet

run: build
	@echo "Running spectro-v3..."
	$(BUILD_DIR)/qt6_gui/spectro

bench: build
	@echo "Running benchmarks..."
	$(BUILD_DIR)/qt6_gui/tests/test_spectrogram_view benchmark
