# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [0.3.0] - 2026-08-16

### Added
- FRDM-MCXN947 board support (`nxp:mcx:frdm_mcxn947`). Supported: GPIO, interrupts, `Serial` (USB), `Wire`/`Wire1` (I2C/I3C), `SPI`, `String`, `analogRead`, `analogWrite`, `tone`/`noTone`, and the UNO-compatible macro set — the same feature set as FRDM-MCXA153. `analogRead` covers A2-A5 only (A0/A1 aren't wired on this board)
- FRDM-MCXN947: independent SPI/I2C/UART instances on the MikroBus header — `SPI1` (`MB_MOSI`/`MB_MISO`/`MB_SCK`/`MB_CS`), `Wire2` (`MB_SDA`/`MB_SCL`), and `Serial1` (`MB_TX`/`MB_RX`) — each on its own peripheral, so all can be used alongside `SPI`/`Wire`/`Serial` in the same sketch. `Serial1` is on the MikroBus header rather than D0/D1: those pins share their only UART-capable peripheral (FlexComm2) with `Wire`, so the two can't coexist there, but MikroBus's `MB_TX`/`MB_RX` have a free FlexComm of their own
- FRDM-MCXA153: independent SPI instance on the MikroBus header — `SPI1` (`MB_MOSI`/`MB_MISO`/`MB_SCK`/`MB_CS`, own peripheral `LPSPI0`, vs. `SPI`'s `LPSPI1`), plus confirmed plain GPIO on all 12 MikroBus pins. Unlike N947, this chip has only one physical I2C peripheral and its existing `Serial1` (D0/D1) already shares its only UART route with the MikroBus pins, so an independent `Wire2`/second `Serial1` isn't possible here — MikroBus I2C/UART can only be alternate, mutually-exclusive pin routings for `Wire`/`Serial1`
- Bare `MOSI`/`MISO`/`SCK` pin macros (both boards), aliased to each board's default SPI pins — standard on every other Arduino core and required by third-party libraries (e.g. the official `SD` library) that reference them directly (fixes [#1](https://github.com/teddokano/mcx-arduino-core/issues/1))
- `Print::setWriteError`/`getWriteError`/`clearWriteError` (both boards), matching ArduinoCore-API's placement/signatures — needed by third-party libraries that derive from `Print` and track write failures this way (e.g. the official `SD` library's `SdFile`/`File`) (fixes [#3](https://github.com/teddokano/mcx-arduino-core/issues/3))
- Per-board `F_CPU`, FPU flags, and upload target are now configurable in `boards.txt` rather than hardcoded, as part of adding the second board

### Changed
- Board display names in the Boards Manager/IDE picker changed from "FRDM-MCXA153 (NXP Cortex-M33)" / "FRDM-MCXN947 (NXP Cortex-M33)" to "... (mcx-arduino-core)", so this core is distinguishable from other packages offering the same boards (e.g. Zephyr's)

### Fixed
- `upload.sh`/`upload.bat` always flashed using the FRDM-MCXA153 LinkServer target regardless of which board was selected — harmless before there was only one board, but would have silently flashed the wrong device once a second board existed
- Both boards: `pinMode()` never touched a pin's PORT mux, only its GPIO direction/data registers — harmless for pins that boot up already muxed to GPIO (true almost everywhere), but N947's `MB_RX`/`MB_TX` boot pre-muxed to the on-board I3C sensor, so `digitalWrite` on them was silently a no-op. `pinMode()` now explicitly reclaims ALT0 (GPIO) on every call
- FRDM-MCXA153: the newly-added `SPI1`'s peripheral (`LPSPI0`) had no clock attached in `init_mcu()` — `SPI`'s `LPSPI1` clock setup was there from the start, but `LPSPI0` had simply never been used by anything before now, so nothing had ever wired it up. CS toggled correctly (plain GPIO) but MOSI/MISO/SCK carried no signal at all
- Both boards: the `SPI` class muxed its `cs` constructor pin onto the LPSPI peripheral's hardware PCS function, so any sketch managing its own CS pin via `pinMode()`/`digitalWrite()` (the standard Arduino pattern, used by every real SPI device library) got silently overridden the moment `SPI.begin()` ran. Worse, on a bus shared by multiple devices with separate CS pins (e.g. an LCD + an SD card), *every* `SPI.transfer()` call for *any* device auto-pulsed the first device's hardware-PCS pin, regardless of which device the transfer was actually for — corrupting whichever device thought it was deselected. Found via a real display-corruption report (garbled/glitched LCD output while reading from an SD card on the same bus). CS is now always left as plain GPIO, fully sketch-controlled
- `SPI::frequency()`/`mode()`/`bit_order()` did a full `LPSPI_Deinit()`+`LPSPI_MasterInit()` (peripheral clock off/on, every register rebuilt from scratch) on every settings change, instead of reconfiguring just the affected registers in place — costly for a sketch alternating two `SPISettings` on one bus every loop iteration (fixes [#2](https://github.com/teddokano/mcx-arduino-core/issues/2))
- `SPI.transfer(uint8_t)` (scalar single-byte transfer) routed through the same generic blocking-transfer path as bulk transfers, paying its full disable/flush-FIFO/clear-status-flags/re-enable overhead (~40us measured) for a single byte — dominant cost for callers that move data one byte at a time in a loop, as the standard Arduino `SD` library does for every byte it reads/writes. Added a fast dedicated path that skips the unneeded per-call reset
- FRDM-MCXN947: `analogWrite`'s pins are now named `PWM0`-`PWM5`, matching FRDM-MCXA153, instead of `PWM_0`-`PWM_5` — `io.h` now reclaims the two colliding SDK macro names (`PWM0`/`PWM1`, the chip's own FlexPWM peripheral instance pointers) via `#undef` before redefining them as pin numbers, the same technique already used elsewhere in this codebase for an identical collision. Sketches written against `PWM0`-`PWM5` now compile unmodified on both boards

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
