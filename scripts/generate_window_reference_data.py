#!/usr/bin/env python3
# Spectro-v3 -- Real-time spectrum analyzer
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2025-2026 Chris "Kai" Frederick

"""
Generate reference data for FFTWindow unit tests.

This script computes the expected window coefficients for various window types
and prints them in a format suitable for inclusion in C++ test files.


Usage:
    python3 scripts/generate_window_reference_data.py

Output:
    C++ constexpr arrays printed to stdout.

Requirements:
    - Python 3.6+
    - scipy
"""

from scipy import signal


def format_cpp_vector(values, name):
    """Format array values as C++ vector."""
    output = f"const std::vector<float> {name} = {{"
    
    # Format values 4 per line
    for i, v in enumerate(values):
        if i % 4 == 0:
            output += "\n    " 
        output += f"{v:.7f}f, "
    output += "\n};\n"
    return output


def main():
    """Generate reference coefficients for all window types."""
    
    window_types = [
        ('hann', 'Hann'),
        ('hamming', 'Hamming'),
        ('blackman', 'Blackman'),
        ('blackmanharris', 'BlackmanHarris'),
    ]
    
    for scipy_name, cpp_name in window_types:
        coeffs = signal.windows.get_window(scipy_name, 8, True)
        print(format_cpp_vector(coeffs, f"kExpected{cpp_name}Window8"))

if __name__ == '__main__':
    main()
