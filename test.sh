#!/bin/bash

set -ex
#cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug

cmake --build build --config Debug --target all -j 4

build/tests/spectro_tests
