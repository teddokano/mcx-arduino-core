# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased] — 0.2.0

### Added
- `analogRead` (LPADC, `A0`-`A3`, 10bit) and `analogWrite` (FlexPWM0, `PWM0`-`PWM5`, fixed 1kHz period)
- `millis()` / `micros()` (SysTick 1ms tick + DWT cycle counter)
- `tone()` / `noTone()` (CTIMER0, any digital pin, one tone at a time)
- `Wire1` — I3C peripheral driven in I2C-compatibility mode, used for the on-board P3T1755 temperature sensor
- `Serial1` — hardware UART on `D0`(RX)/`D1`(TX), independent of the USB-bridged `Serial`
- `shiftOut()` / `shiftIn()`, `pulseIn()` / `pulseInLong()`, `random()` / `randomSeed()`
- UNO R3/R4 compatibility macros and constants: math constants (`PI`, `HALF_PI`, `TWO_PI`, `DEG_TO_RAD`, `RAD_TO_DEG`, `EULER`), `radians()`/`degrees()`, `LSBFIRST`/`MSBFIRST`/`SERIAL`/`DISPLAY`, `boolean`/`byte`/`word`, `min()`/`max()`, `abs()`/`constrain()`/`sq()`, bit-manipulation macros (`bitRead`, `bitSet`, `bitClear`, `bitToggle`, `bitWrite`, `bit`, `lowByte`, `highByte`), `interrupts()`/`noInterrupts()`, `map()`
- `Serial.print()`/`println()` overloads for `long long`/`unsigned long long`, resolving an ambiguous-overload compile error on `Serial.print(time_t)`

### Fixed
- BusFault when using `Wire1` (I3C) together with `tone()`, caused by an unset base pointer in the I2C base class being reached through a hidden (not overridden) virtual method
- `Serial1` RX not working: input buffer was never enabled on `D0`, and the RX interrupt/ring buffer was never activated
- `Serial.available()` returning only 0/1 instead of the actual number of buffered bytes
- Missing `-lm` in the link recipe, causing link errors for any sketch using standard math functions (`ceilf`, `floorf`, etc.)

## [0.1.8] - 2026-04-06

### Fixed
- `TwoWire::requestFrom` return value

## [0.1.7] - 2026-04-06

### Fixed
- Wire (I2C) hang-up on NAK
- SPI: CS now stays asserted for the full duration of a multi-byte transfer
- Enabled multiple class instances to co-exist by adjusting heap size
- `Serial` instance changed to a static instance

### Changed
- Reorganized `arduino_serial.cpp`/`.h` and adjusted `arduino_i2c.cpp`/linker script paths

## [0.1.6] - 2026-03-30

### Fixed
- Arduino API naming and compatibility issues (`pinMode`, `LED_BUILTIN`, and other API prototype mismatches)

## [0.1.5] - 2026-03-30

### Fixed
- `Wire`/`SPI` global instances changed to lazy initialization in `begin()`, fixing an initialization-order issue under Arduino IDE's `--no-whole-archive` linking
- SPI: only reconfigure clock/mode in `beginTransaction()` when settings actually changed, preventing a spurious CS pulse on LPSPI reinitialization
- `Serial.print(double)` reimplemented with integer arithmetic (`nano.specs` doesn't support float in `snprintf`)
- `Serial.printf()` reimplemented with a custom parser handling `%f`/`%g`/`%e`

### Added
- `digits` parameter to `Serial.print(double, digits)` / `println(double, digits)`

## [0.1.4] - 2026-03-29

### Fixed
- Interrupt handling

## [0.1.3] - 2026-03-28

### Added
- VID/PID for automatic board detection

## [0.1.2] - 2026-03-28

### Added
- Sketch upload support

## [0.1.1] - 2026-03-28

### Fixed
- Build issues: `sbrk`, linker script, `operator new`/`delete`, `arduino_io` without `std::map`

## [0.1.0] - 2026-03-28

### Added
- Initial experimental release for FRDM-MCXA153
