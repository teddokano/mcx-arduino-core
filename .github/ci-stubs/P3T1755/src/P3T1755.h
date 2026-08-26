/** CI-only compile stub for the real P3T1755 library.
 *
 *  The real library lives in a separate repo (not vendored here, and
 *  gitignored under examples/tests/ when cloned locally for interactive
 *  testing) -- see examples/release_check/README.md. This stub exists
 *  only so the handful of examples that #include <P3T1755.h> can be
 *  compile-checked in CI without that external dependency. It proves
 *  nothing about sensor behavior; only real hardware verification with
 *  the real library does that.
 */
#pragma once
#include <Wire.h>

class P3T1755 {
public:
  P3T1755(TwoWire &wire, uint8_t address) {
    (void)wire;
    (void)address;
  }

  float temp() {
    return 25.0f;
  }
};
