/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  Real Arduino sketches (and many third-party I2C libraries) expect to
 *  find the I2C API via `#include <Wire.h>` (capital-cased, matching every
 *  official Arduino core's convention), often without also including
 *  <Arduino.h> first. This project's actual TwoWire implementation lives
 *  in arduino_i2c.h -- this is a thin, filename-only wrapper so that
 *  convention holds here too (same pattern as SPI.h/arduino_spi.h).
 *
 *  This file previously reused Arduino.h's own include guard
 *  (R01LIB_ARDUINO_H) and duplicated a chunk of its includes plus
 *  setup()/loop()/delay() declarations -- harmless in every .ino (which
 *  always gets <Arduino.h> auto-prepended before any #include <Wire.h> of
 *  its own, so the guard just silently skipped this file's body), but a
 *  real "Wire.h: No such file or directory"-shaped trap waiting for any
 *  translation unit that included <Wire.h> on its own without
 *  <Arduino.h> already in scope -- a common pattern for third-party I2C
 *  device libraries. Given its own guard now, so it works correctly
 *  however it's included.
 */

#ifndef R01LIB_WIRE_WRAPPER_H
#define R01LIB_WIRE_WRAPPER_H

#include "arduino_i2c.h"

#endif // !R01LIB_WIRE_WRAPPER_H
