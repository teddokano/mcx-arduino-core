# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- FRDM-MCXN947 board support (`nxp:mcx:frdm_mcxn947`). Supported: GPIO, interrupts, `Serial` (USB), `Wire`/`Wire1` (I2C/I3C), `SPI`, `String`, `analogRead`, `analogWrite`, `tone`/`noTone`, and the UNO-compatible macro set — the same feature set as FRDM-MCXA153. `Serial1` (D0/D1 hardware UART) is not available on this board: those pins share their only UART-capable peripheral (FlexComm2) with `Wire`, so the two can't coexist in one sketch. `analogRead` covers A2-A5 only (A0/A1 aren't wired on this board)
- Per-board `F_CPU`, FPU flags, and upload target are now configurable in `boards.txt` rather than hardcoded, as part of adding the second board

### Fixed
- `upload.sh`/`upload.bat` always flashed using the FRDM-MCXA153 LinkServer target regardless of which board was selected — harmless before there was only one board, but would have silently flashed the wrong device once a second board existed

## [0.2.2] - 2026-08-13

### Fixed
- Build failure on Linux (and any case-sensitive filesystem): the core's actual header file was named `arduino.h`, but every sketch gets `#include <Arduino.h>` auto-prepended by the Arduino IDE/arduino-cli. This only ever worked by accident on macOS/Windows, whose default filesystems don't distinguish case — present since 0.1.0, only caught now with the first real Linux build attempt. Renamed to `Arduino.h`. A second instance of the same bug was found and fixed at the same time: `#include <SPI.h>` (used by most SPI examples) only ever resolved to r01lib's unrelated low-level `spi.h` by the same case-insensitive accident; added a real `SPI.h` (wrapping the existing `arduino_spi.h`, which is the file that actually implements `SPIClass`) after renaming r01lib's low-level SPI class out of the way to `r01lib_spi.h`

## [0.2.1] - 2026-08-12

### Added
- `String` class (original implementation, not a WString port): concatenation (including `long long`/`unsigned long long` and `F("...")`), `substring`/`indexOf`/`replace`, `toInt`/`toFloat`, `getBytes`/`toCharArray`, free `operator+` for all numeric types
- Real `Print`/`Stream`/`Printable` abstract base classes (original implementation, not a port of ArduinoCore-avr/API's LGPL 2.1 `Print`/`Stream`). `Serial`/`Serial1` now derive from `Stream`; any class can inherit `Print` directly (no hardware required) and get `print()`/`println()` for free by implementing `write(uint8_t)`. `print()`/`println()` now return `size_t` (bytes written), matching real Arduino
- `Serial.flush()`, `Serial.peek()`, `Serial.availableForWrite()`
- Serial/Stream helpers: `setTimeout`, `readBytes`, `readBytesUntil`, `readString`, `readStringUntil`, `parseInt`, `parseFloat`, `find`, `find(target, length)`, `findUntil`
- `delayMicroseconds()`, `detachInterrupt()`
- `Wire.setClock()`, `Wire.end()`
- `SPI.end()`, `SPI.transfer16()`, `SPI.usingInterrupt()`/`notUsingInterrupt()`, legacy `SPI.setBitOrder()`/`setDataMode()`/`setClockDivider()`
- `analogReference()`, `analogReadResolution()`, `analogWriteResolution()`
- `INPUT_PULLDOWN`, `OUTPUT_OPENDRAIN` pin modes
- `NOT_AN_INTERRUPT`, `digitalPinToPort`/`digitalPinToBitMask`/`portOutputRegister`/`portInputRegister`/`portModeRegister` (fast-GPIO/bit-banging support)
- `PROGMEM`/`pgm_read_byte`/etc./`PSTR`, `F("...")`/`__FlashStringHelper`, `ARDUINO`/`ARDUINO_ARCH_MCX`/`ARDUINO_FRDM_MCXA153` macros
- `F_CPU`, `clockCyclesPerMicrosecond()`/`clockCyclesToMicroseconds()`/`microsecondsToClockCycles()`, `BitOrder` typedef
- `yield()`, character functions (`isAlpha`, `isDigit`, `isSpace`, etc.)
- Improved Linux `LinkServer` discovery in `upload.sh` (adapted from ArduinoCore-zephyr, Apache 2.0)

### Fixed
- SPI `bitOrder` in `SPISettings` was declared but never actually applied to hardware
- `Serial.print()`/`println()` with `BIN` base silently printed decimal instead (both `Serial` and `String`)
- `Serial.write()` missing overloads due to C++ name hiding (only `write(uint8_t)` was reachable)
- `attachInterrupt(pin, isr, LOW)` silently behaved as `RISING` due to a numeric constant collision with the digital-level `LOW` constant
- BusFault/hang in `Wire.end()` on an I3C-backed `TwoWire` (I2C's destructor dereferenced an intentionally-uninitialized hardware pointer)
- Third-party libraries that inherit `Print` directly or take `Stream&` (ArduinoJson, LiquidCrystal, DHT sensor library, etc.) failed to compile — no real `Print`/`Stream` base classes existed

### Changed
- `README.md`'s full API compatibility table moved to `API_COMPATIBILITY.md`

## [0.2.0] - 2026-08-12

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
