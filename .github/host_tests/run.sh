#!/usr/bin/env bash
# Builds and runs the host tests: the core's portable files (Print,
# String), compiled with the build machine's own C++ compiler and checked
# against what the boards print. No toolchain, no board, a few seconds.
# The regression check runs it in its hygiene job; run it locally after
# changing those files.
#
# Usage: .github/host_tests/run.sh   (CXX picks the compiler; c++ by default)
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CORE="${HERE}/../../hardware/nxp/mcx/cores/arduino/arduino_api"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

"${CXX:-c++}" -std=c++20 -Wall -Wextra -Wno-unused-parameter \
  -I "${HERE}/stub" -I "${CORE}" \
  -o "${OUT}/test_print_string" \
  "${HERE}/test_print_string.cpp" "${CORE}/Print.cpp" "${CORE}/arduino_string.cpp"

"${OUT}/test_print_string"
