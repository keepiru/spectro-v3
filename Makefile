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
# Detect if running in Docker and use separate build directory so we don't mess
# up clangd analysis on the host.
ifeq ($(shell test -f /.dockerenv && echo yes),yes)
    BUILD_DIR := build-docker
else
    BUILD_DIR := build
endif
UNITY_DIR := build-unity
RELEASE_DIR := build-release
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
configure: $(BUILD_DIR)/compile_commands.json

$(BUILD_DIR)/compile_commands.json:
	cmake --preset=default -B $(BUILD_DIR)


# Configure CMake for unity builds (faster builds for development)
configure-unity: $(UNITY_DIR)/compile_commands.json

$(UNITY_DIR)/compile_commands.json:
	cmake --preset=unity -B $(UNITY_DIR)

# Build the project (configures if needed)
build: configure
	cmake --build $(BUILD_DIR) -j $(JOBS)

# Build the project with unity
unity: configure-unity
	cmake --build $(UNITY_DIR) -j $(JOBS)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(UNITY_DIR) $(RELEASE_DIR)

# Run tests via CTest
test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure --progress

# Run single test by name pattern (usage: make test-one NAME=FFTProcessor)
test-one: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure -R "$(NAME)"

# Release build
release:
	rm -rf $(RELEASE_DIR)
	cmake --preset=release -B $(RELEASE_DIR)
	cmake --build $(RELEASE_DIR) -j $(JOBS)

# Lint with clang-tidy
lint: configure
	find dsp/ qt6_gui/ -name '*.cpp' | xargs run-clang-tidy -p $(BUILD_DIR) -use-color -quiet

# Lint specific files (usage: make lint-file FILE="file1.cpp file2.cpp")
lint-files: configure
	run-clang-tidy -p $(BUILD_DIR) -use-color -quiet $(filter-out $@,$(MAKECMDGOALS))

# Lint with automatic fixes (use with caution)
lint-fix: configure
	find dsp/ qt6_gui/ -name '*.cpp' | xargs run-clang-tidy -p $(BUILD_DIR) -use-color -fix -quiet

# Lint files changed in git
lint-changed: configure
	git diff --name-only HEAD | xargs run-clang-tidy -p $(BUILD_DIR) -use-color -quiet

# Lint files changed in git
lint-fix-changed: configure
	git diff --name-only HEAD | xargs run-clang-tidy -p $(BUILD_DIR) -use-color -fix -quiet

run: build
	$(BUILD_DIR)/qt6_gui/spectro

bench: build
	$(BUILD_DIR)/qt6_gui/tests/test_spectrogram_view benchmark
