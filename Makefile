# Makefile for spectro-v3 routine tasks
# 
# This wraps CMake commands for convenience. The actual build logic
# remains in CMakeLists.txt - this just provides shorter commands.

# Suppress "Entering/Leaving directory" messages
MAKEFLAGS += --no-print-directory

# Configuration
LOCAL_BUILD_DIR := build
BUILD_DIR := build-docker
BUILD_TYPE := Debug
JOBS := $(shell nproc 2>/dev/null || echo 4)
DOCKER_IMAGE := spectro-v3-builder
DOCKER_RUN := docker run --rm -it --user $(shell id -u):$(shell id -g) -v $(PWD):/build -w /build $(DOCKER_IMAGE)

.PHONY: all build configure clean rebuild test test-verbose test-direct test-one \
        tdd release lint lint-fix help shell build-image

# Default target
all: build

# Build the Docker image
build-image:
	@echo "Building Docker image..."
	docker build -q -t $(DOCKER_IMAGE) .

# Configure CMake (run once or after CMakeLists.txt changes)
configure: build-image
	@echo "Configuring CMake..."
	$(DOCKER_RUN) cmake -B $(BUILD_DIR) -S . -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

# Build the project (configures if needed)
build: build-image | $(BUILD_DIR)/Makefile
	@echo "Building..."
	$(DOCKER_RUN) cmake --build $(BUILD_DIR) --config $(BUILD_TYPE) -j $(JOBS)

# Build the project without Docker (configures if needed)
build-local: $(LOCAL_BUILD_DIR)/Makefile
	@echo "Building..."
	cmake --build $(LOCAL_BUILD_DIR) --config $(BUILD_TYPE) -j $(JOBS)

# Ensure build directory exists and is configured
$(BUILD_DIR)/Makefile:
	@$(MAKE) configure

# Clean build artifacts
clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR)

# Full rebuild (clean + configure + build)
rebuild: clean configure build

# Run tests via CTest
test: build
	@echo "Running tests..."
	$(DOCKER_RUN) ctest --test-dir $(BUILD_DIR) --output-on-failure

# Run tests with verbose output (useful for TDD)
test-verbose: build
	@echo "Running tests (verbose)..."
	$(DOCKER_RUN) ctest --test-dir $(BUILD_DIR) --output-on-failure --verbose

# Run test executable directly (faster iteration during TDD)
test-direct: build
	@echo "Running tests directly..."
	$(DOCKER_RUN) $(BUILD_DIR)/dsp/tests/spectro_dsp_tests

# Run single test by name pattern (usage: make test-one NAME=FFTProcessor)
test-one: build
	@echo "Running tests matching '$(NAME)'..."
	$(DOCKER_RUN) ctest --test-dir $(BUILD_DIR) --output-on-failure -R "$(NAME)"

# Quick TDD cycle: build and test in one command
tdd: test-direct

# Release build
release: build-image
	@echo "Building release..."
	$(DOCKER_RUN) cmake -B $(BUILD_DIR) -S . -DCMAKE_BUILD_TYPE=Release
	$(DOCKER_RUN) cmake --build $(BUILD_DIR) --config Release -j $(JOBS)

# Lint with clang-tidy
lint: build-image
	@echo "Running clang-tidy on source files..."
	$(DOCKER_RUN) run-clang-tidy -p $(BUILD_DIR) -use-color -quiet

# Lint with automatic fixes (use with caution)
lint-fix: build-image
	@echo "Running clang-tidy with automatic fixes..."
	$(DOCKER_RUN) run-clang-tidy -p $(BUILD_DIR) -use-color -fix -quiet

# Open interactive shell in Docker container
shell: build-image
	@echo "Starting Docker shell..."
	$(DOCKER_RUN) /bin/bash

run: build
	@echo "Running spectro-v3..."
	$(BUILD_DIR)/qt6_gui/spectro

# Help target
help:
	@echo "Spectro-v3 Build System (Docker-based)"
	@echo ""
	@echo "Build Targets:"
	@echo "  make              - Build the project (default)"
	@echo "  make build-image  - Build the Docker image"
	@echo "  make configure    - Configure CMake"
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
	@echo ""
	@echo "Code Quality:"
	@echo "  make lint         - Run clang-tidy on all source files"
	@echo "  make lint-fix     - Run clang-tidy with automatic fixes"
	@echo ""
	@echo "Development:"
	@echo "  make shell        - Open interactive shell in Docker"
	@echo ""
	@echo "Configuration:"
	@echo "  BUILD_TYPE=$(BUILD_TYPE) (Debug/Release/RelWithDebInfo)"
	@echo "  JOBS=$(JOBS) (parallel jobs)"
	@echo "  DOCKER_IMAGE=$(DOCKER_IMAGE)"
