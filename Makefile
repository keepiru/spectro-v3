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
    ANALYSIS_DIR := build-docker
else
    BUILD_DIR := build-unity
    ANALYSIS_DIR := build
endif
BUILD_TYPE := Debug
JOBS := $(shell nproc 2>/dev/null || echo 4)

.PHONY: all build configure clean rebuild test test-one \
        tdd release lint lint-fix-changed lint-fix lint-files help run bench \
        configure-analysis

# Default target
all: build

# Configure CMake (run once or after CMakeLists.txt changes)
configure:
	@echo "Configuring CMake..."
	cmake --preset=default -B ${BUILD_DIR}

# Configure for static analysis (without unity builds).  run-clang-tidy needs
# compile_commands.json to be present and unity-free.
configure-analysis:
	@echo "Configuring CMake for static analysis (no unity builds)..."
	cmake --preset=analysis -B ${ANALYSIS_DIR}

# Build the project (configures if needed)
build: | $(BUILD_DIR)/Makefile
	@echo "Building..."
	cmake --build $(BUILD_DIR) --config $(BUILD_TYPE) -j $(JOBS)

# Ensure build directory exists and is configured
$(BUILD_DIR)/Makefile:
	$(MAKE) configure

# Clean build artifacts
clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR) $(ANALYSIS_DIR)

# Full rebuild (clean + configure + build)
rebuild: clean configure build

# Run tests via CTest
test: build
	@echo "Running tests..."
	ctest --test-dir $(BUILD_DIR) --output-on-failure --progress

# Run single test by name pattern (usage: make test-one NAME=FFTProcessor)
test-one: build
	@echo "Running tests matching '$(NAME)'..."
	ctest --test-dir $(BUILD_DIR) --output-on-failure -R "$(NAME)"

# Release build
release:
	@echo "Building release..."
	cmake --preset=release -B ${BUILD_DIR}
	cmake --build $(BUILD_DIR) --config Release -j $(JOBS)

# Lint with clang-tidy
lint: $(ANALYSIS_DIR)/compile_commands.json
	@echo "Running clang-tidy on source files..."
	find dsp/ qt6_gui/ -name '*.cpp' | xargs run-clang-tidy -p $(ANALYSIS_DIR) -use-color -quiet

# Lint specific files (usage: make lint-file FILE="file1.cpp file2.cpp")
lint-files: $(ANALYSIS_DIR)/compile_commands.json
	@echo "Running clang-tidy..."
	run-clang-tidy -p $(ANALYSIS_DIR) -use-color -quiet $(filter-out $@,$(MAKECMDGOALS))

# Lint with automatic fixes (use with caution)
lint-fix: $(ANALYSIS_DIR)/compile_commands.json
	@echo "Running clang-tidy with automatic fixes..."
	find dsp/ qt6_gui/ -name '*.cpp' | xargs run-clang-tidy -p $(ANALYSIS_DIR) -use-color -fix -quiet

# Lint files changed in git
lint-changed: $(ANALYSIS_DIR)/compile_commands.json
	@echo "Running clang-tidy on changed files..."
	git diff --name-only HEAD | xargs run-clang-tidy -p $(ANALYSIS_DIR) -use-color -quiet

# Lint files changed in git
lint-fix-changed: $(ANALYSIS_DIR)/compile_commands.json
	@echo "Running clang-tidy on changed files..."
	git diff --name-only HEAD | xargs run-clang-tidy -p $(ANALYSIS_DIR) -use-color -fix -quiet

# Ensure analysis build is configured
$(ANALYSIS_DIR)/compile_commands.json:
	$(MAKE) configure-analysis

run: build
	@echo "Running spectro-v3..."
	$(BUILD_DIR)/qt6_gui/spectro

bench: build
	@echo "Running benchmarks..."
	$(BUILD_DIR)/qt6_gui/tests/test_spectrogram_view benchmark

# Help target
help:
	@echo "Spectro-v3 Build System"
	@echo ""
	@echo "Build Targets:"
	@echo "  make              - Build the project (default)"
	@echo "  make configure    - Configure CMake"
	@echo "  make configure-analysis - Configure for static analysis (no unity builds)"
	@echo "  make build        - Build the project"
	@echo "  make clean        - Remove build directory"
	@echo "  make rebuild      - Clean + configure + build"
	@echo "  make release      - Build with Release configuration"
	@echo ""
	@echo "Test Targets (TDD Workflow):"
	@echo "  make test         - Run all tests via CTest"
	@echo "  make test-verbose - Run tests with verbose output"
	@echo "  make test-direct  - Run test executable directly (faster)"
	@echo "  make test-one NAME=<pattern> - Run tests matching pattern"
	@echo "  make tdd          - Quick build + test cycle"
	@echo "  make bench        - Run benchmarks"
	@echo ""
	@echo "Code Quality:"
	@echo "  make lint         - Run clang-tidy on all source files"
	@echo "  make lint-files <file1> <file2> - Run clang-tidy on specific files"
	@echo "  make lint-fix     - Run clang-tidy with automatic fixes"
	@echo ""
	@echo "Development:"
	@echo "  make run          - Run application"
	@echo ""
	@echo "Configuration:"
	@echo "  BUILD_TYPE=$(BUILD_TYPE) (Debug/Release/RelWithDebInfo)"
	@echo "  JOBS=$(JOBS) (parallel jobs)"
