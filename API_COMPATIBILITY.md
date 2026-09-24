# Supported Arduino APIs

Status of Arduino-standard APIs on this core, covering both supported
boards (FRDM-MCXA153 and FRDM-MCXN947). The two boards share the same API
surface except where a row below notes a difference. See the main
[README](README.md) for board setup, pin mapping, and general usage.

## GPIO & Interrupts

| API | Status | Notes |
|-----|--------|-------|
| `pinMode` | ✅ | `INPUT` / `OUTPUT` / `INPUT_PULLUP` / `INPUT_PULLDOWN` / `OUTPUT_OPENDRAIN` |
| `digitalWrite` / `digitalRead` | ✅ | |
| `attachInterrupt` | ✅ | RISING / FALLING / CHANGE / LOW (level-triggered, fires repeatedly while held) |
| `detachInterrupt` | ✅ | |
| `digitalPinToInterrupt` / `NOT_AN_INTERRUPT` | ✅ | Every valid GPIO pin on this MCU supports interrupts, so `digitalPinToInterrupt()` never actually returns `NOT_AN_INTERRUPT` -- it's provided so sketches that check for it still compile |
| `digitalPinToPort` / `digitalPinToBitMask` / `portOutputRegister` / `portInputRegister` / `portModeRegister` | ✅ | For fast-GPIO/bit-banging libraries; pin must have `pinMode()` called first |
| `NUM_DIGITAL_PINS` | ✅ | `16` (D0-D13, D18, D19), same on both boards |
| `NUM_ANALOG_INPUTS` | ✅ | `6` on FRDM-MCXA153, `4` on FRDM-MCXN947 (A0/A1 unavailable there — see Analog section below) |
| `digitalPinHasPWM(pin)` | ✅ | True only for `PWM0`-`PWM5` — this board's PWM capability lives on those dedicated pins, not on the D-pins the way it does on AVR boards |
| `PIN_WIRE_SDA` / `PIN_WIRE_SCL` / `PIN_SPI_SS` / `PIN_SPI_MOSI` / `PIN_SPI_MISO` / `PIN_SPI_SCK` | ✅ | Standard `pins_arduino.h`-style aliases (AVR/SAMD convention) for this board's `I2C_SDA`/`I2C_SCL`/`ARD_CS`/`ARD_MOSI`/`ARD_MISO`/`ARD_SCK` |
| `SERIAL_PORT_MONITOR` / `SERIAL_PORT_HARDWARE` / `SERIAL_PORT_HARDWARE_OPEN` | ✅ | Standard AVR/SAMD-convention aliases — `Serial` / `Serial1` / `Serial1`. `SERIAL_PORT_USBVIRTUAL` is deliberately not defined: this board's `Serial` is a hardware UART routed through an external USB-CDC bridge chip, not a native-USB virtual port |

## Serial

| API | Status | Notes |
|-----|--------|-------|
| `Serial.begin` / `print` / `println` / `printf` | ✅ | |
| `Serial.begin(baud, config)` | ✅ | Added in v0.7.0. `SERIAL_7N1` through `SERIAL_8O2`: 7 or 8 data bits, no/even/odd parity, 1 or 2 stop bits, with the same values as ArduinoCore-API, so the `SERIAL_DATA_`/`SERIAL_PARITY_`/`SERIAL_STOP_BIT_` parts work too. A byte that fails the parity check is dropped, as on AVR. With 7 data bits, bit 7 of what is read is 0. The LPUART can't do 5 or 6 data bits, mark/space parity or 1.5 stop bits, so `SERIAL_5N1` and the rest aren't defined and a sketch using one fails to build, rather than talking to its device in the wrong format. `begin(baud)` alone goes back to `SERIAL_8N1` |
| `Serial.end` | ✅ | Added in v0.7.0. Waits for pending output, stops receiving, drops what wasn't read, and hands the TX/RX pins back as plain GPIO inputs. `begin()` starts it again |
| `serialEvent()` / `serialEvent1()` | ✅ | Added in v0.7.0. Called after each `loop()` while `Serial`/`Serial1` has something to read, as on AVR. Before v0.7.0 a sketch could define them, but they were never called |
| `Serial.read` / `available` / `write` | ✅ | `write` has all 4 standard overloads (`uint8_t`, `const char*`, `(const uint8_t*, size_t)`, `(const char*, size_t)`), plus `write(int)` and the other integer types, which send the low byte as on AVR. Before v0.7.0 `Serial.write(0)` failed to build: `0` fit `uint8_t` and `const char*` equally well |
| `Serial.print`/`println` with `BIN` base | ✅ | Fixed in v0.2.1 — previously silently printed decimal instead of binary |
| `Serial.print`/`println` of a `float`/`double` | ✅ | Rounded to the requested decimals (2 by default), as on other cores. Fixed in v0.7.0: it used to truncate, so `print(1.999)` gave `1.99` instead of `2.00`. Same edge cases as ArduinoCore-avr/-API: no decimal point for 0 decimals, `nan`, `inf`, and `ovf` once the integer part passes 32 bits |
| `Serial.flush` | ✅ | Blocks until the hardware finishes shifting out the last byte, not just until the software TX buffer is empty |
| `Serial.peek` | ✅ | Only meaningful after `begin()` (always the case for `Serial`/`Serial1`), since it reads the RX ring buffer |
| `Serial.setTimeout` / `readBytes` / `readBytesUntil` / `readString` / `readStringUntil` / `parseInt` / `parseFloat` / `find` | ✅ | Polled, `millis()`-based timeout (default 1000ms) |
| `Serial.find(target, length)` / `findUntil(target, terminator)` | ✅ | Added in v0.2.1 |
| `Serial.availableForWrite` | ✅ | Added in v0.2.1 — free bytes in the TX ring buffer (max 255) |
| `HardwareSerial` type name | ✅ | Added in v0.7.0. Another name for `SerialClass`, the class of `Serial`/`Serial1`, for libraries that take a `HardwareSerial&`. It is a `typedef`, so a library that forward-declares `class HardwareSerial;` itself still won't compile |
| `Serial1` | ✅ | Hardware UART, separate from USB-bridged `Serial`. On `D0`/`D1` on FRDM-MCXA153, but on the MikroBus header (`MB_TX`/`MB_RX`) on FRDM-MCXN947: there `D0`/`D1`'s only UART-capable peripheral is the same FlexComm instance `Wire` uses for I2C, and a FlexComm can only be one peripheral mode at a time. The MikroBus pins are also `Wire1`'s, so on that board `Serial1` and `Wire1` take turns rather than run together |

## Wire (I2C / I3C)

| API | Status | Notes |
|-----|--------|-------|
| `Wire.begin` / `beginTransmission` / `endTransmission` | ✅ | |
| `Wire.write` / `read` / `requestFrom` / `available` | ✅ | `write()` returns 1 per byte, and 0 once the 128-byte buffer (`WIRE_BUFFER_SIZE`) is full, setting the write error, as on AVR. Before v0.7.0 it returned the total queued so far and wrote past the end of the buffer. `requestFrom()` of more than 128 bytes is cut down to 128, as AVR cuts down to its 32; before, it overran the buffer too |
| `Wire` as a `Stream` (`peek`, `flush`, `print`, `readBytes`, passing `Wire` as a `Stream&`) | ✅ | Added in v0.7.0. `TwoWire` derives from `Stream`, as in the official cores. `print()` goes into the transaction being built; the `Stream` helpers read what the last `requestFrom()` got. Outgoing and incoming bytes have separate buffers. Before v0.7.0 they shared one, so starting a transaction before reading everything `requestFrom()` got made `available()` and `read()` go wrong. `flush()` does nothing, as on AVR: every transfer is over when `endTransmission()`/`requestFrom()` returns |
| `Wire.requestFrom(address, quantity, iaddress, isize, sendStop)` | ✅ | Added in v0.7.0. AVR's five-argument form: writes the register address (`isize` bytes, at most 3, most significant first), then reads after a repeated start. Unlike AVR, it returns 0 without reading when the target doesn't acknowledge the register address |
| `Wire.setClock` | ✅ | |
| `Wire.end` | ✅ | Added in v0.2.1 — releases the I2C/I3C peripheral |
| `Wire.setWireTimeout` / `clearWireTimeoutFlag` / `getWireTimeoutFlag` | ✅ `Wire`/`Wire2` / ❌ `Wire1` | Added in v0.7.0, with `WIRE_HAS_TIMEOUT`. Same signature and defaults as AVR, and disabled until called. What is timed is how long SCL or SDA stays low in one stretch (the LPI2C's hardware pin-low timeout), not the whole transaction. The hardware caps the limit, and on FRDM-MCXA153 the cap shrinks with bus speed: ~87ms at 100kHz but ~21.8ms at 400kHz, so even the 25ms default is clamped there. FRDM-MCXN947 is ~87ms at every speed. On a timeout `endTransmission()` returns `138` (the SDK's `kStatus_LPI2C_PinLowTimeout`, truncated to `uint8_t`) rather than AVR's `5`; this core's other codes aren't AVR's either. A line that is already low when a transfer starts makes it fail at once as busy (`132`) instead, with the flag left clear. The bus recovers once the line is released, with or without `reset_with_timeout`. Not implemented on `Wire1`, which runs on I3C, so it has no effect there. `Wire` is hardware-verified on both boards. `Wire2` runs the same code but hasn't been exercised on hardware |
| I2C slave mode (`Wire.begin(address)`, `onReceive`, `onRequest`) | ❌ | Not supported — master mode only. r01lib has no slave-mode driver, but the MCUXpresso SDK's `fsl_lpi2c` it builds on does (`LPI2C_Slave*`, interrupt-driven, on both boards), so this is Arduino-layer and interrupt plumbing rather than a new low-level driver |
| `Wire1` (I3C, I2C mode) | ✅ | On-board P3T1755 temperature sensor (different physical I3C pins per board -- see the board's pin mapping) |

## SPI

| API | Status | Notes |
|-----|--------|-------|
| `SPI.begin` / `end` / `beginTransaction` / `endTransaction` / `transfer` / `transfer16` | ✅ | `bitOrder` in `SPISettings` is now actually applied to hardware (was silently ignored before v0.2.1). CS is always plain GPIO, fully sketch-controlled via `pinMode`/`digitalWrite` -- never muxed to the LPSPI hardware chip-select function -- so multiple devices can correctly share one bus with separate CS pins (e.g. an LCD + an SD card), matching standard Arduino SPI semantics |
| `SPI.usingInterrupt` / `notUsingInterrupt` | ✅ | No-op — declared for sketch compatibility only |
| `SPI.setBitOrder` / `setDataMode` / `setClockDivider` | ✅ | Legacy pre-1.6 API; `setClockDivider` divides `SPI`'s peripheral input clock, not `F_CPU` |
| Bare `MOSI` / `MISO` / `SCK` pin macros | ✅ | Aliased to this board's default SPI pins (`ARD_MOSI`/`ARD_MISO`/`ARD_SCK`); needed by third-party libraries (e.g. the official `SD` library) that reference them directly |
| Second SPI instance on MikroBus (`SPI1`) | ✅ | `SPI1` on `MB_MOSI`/`MB_MISO`/`MB_SCK`/`MB_CS`, own peripheral (`LPSPI6` on N947, `LPSPI0` on A153), independent of `SPI` — see [PIN_MAPPING_N947.md](PIN_MAPPING_N947.md) / [PIN_MAPPING_A153.md](PIN_MAPPING_A153.md) |

## Timing & Tone

| API | Status | Notes |
|-----|--------|-------|
| `delay` | ✅ | |
| `delayMicroseconds` | ✅ | |
| `millis` / `micros` | ✅ | SysTick(1ms) + DWT cycle counter |
| `tone` / `noTone` | ✅ | CTIMER0, any digital pin, 1 tone at a time |

## Analog I/O

| API | Status | Notes |
|-----|--------|-------|
| `analogRead` | ✅ | LPADC. `A0`-`A3` on A153; `A2`-`A5` on N947 (`A0`/`A1` aren't wired to an ADC channel on that board). 10bit (0-1023) default |
| `analogWrite` (PWM) | ✅ | FlexPWM0 on A153 / FlexPWM1 on N947, 1kHz period by default (see `analogWriteFrequency` below to change it). **Real PWM only on the dedicated `PWM0`-`PWM5` pins** — no `D0`-`D13` pin has a FlexPWM alternate function on N947, and on A153 only `D3`/`D7` do, on the channels `PWM5`/`PWM4` already use, so there is no classic-Arduino `analogWrite(9, ...)` PWM pin on either board. On any other pin `analogWrite()` falls back to `digitalWrite()` — LOW below the midpoint of the current `analogWriteResolution()`, HIGH at or above it — which is what AVR's core does for a pin with no timer behind it. It does **not** produce PWM there, and does not fail either. Dedicated pins named `PWM0`-`PWM5` on both boards -- on N947, `PWM0`/`PWM1` also collide with the chip's own SDK macros for the FlexPWM peripheral instances themselves, so `io.h` explicitly reclaims those two names (`#undef`) before redefining them as pin numbers |
| `analogWriteFrequency(pin, hz)` | ✅ | Fails loudly (`panic()`) on a pin that isn't `PWM0`-`PWM5`: a plain GPIO has no period to set, and unlike `analogWrite()` this is not a call a sketch ported from another board makes by accident. Non-standard extension, not part of the official Arduino API — modeled on Teensy's function of the same name (official Arduino never standardized PWM frequency control). Sets a pin's PWM period; the pin's duty is preserved as an absolute pulse width across the change, not as a ratio, so call this *before* `analogWrite()` to set duty at the new rate. `PWM0`-`PWM5` pair up two-to-a-submodule and share the period register within each pair — see `PIN_MAPPING_A153.md`/`PIN_MAPPING_N947.md` |
| `analogReference` | ✅ | No-op — this board's ADC reference voltage is fixed in hardware. Accepts AVR's `DEFAULT`/`INTERNAL`/`EXTERNAL` and the 32-bit cores' `AR_DEFAULT`/`AR_INTERNAL`/`AR_EXTERNAL` (added in v0.7.0) |
| `analogReadResolution` / `analogWriteResolution` | ✅ | 1-16 bit; defaults match classic Arduino (10bit read / 8bit write) |

## Other Digital I/O Helpers

| API | Status | Notes |
|-----|--------|-------|
| `shiftOut` / `shiftIn` | ✅ | Software bit-banged |
| `pulseIn` / `pulseInLong` | ✅ | |
| `random` / `randomSeed` | ✅ | |

## String, Print & Stream

| API | Status | Notes |
|-----|--------|-------|
| `String` class | ✅ | Original implementation (not a WString port); concatenation (including `long long`/`unsigned long long`, and `F("...")`), `substring`/`indexOf`/`replace`, `toInt`/`toFloat`, `getBytes`/`toCharArray`, free `operator+` for all numeric types and `F("...")`, etc. `reserve()` is a no-op (always allocates exact-fit). `String(double, decimalPlaces)` rounds, as on other cores (it truncated before v0.7.0), and prints large values in full. Unlike ArduinoCore-avr/-API, `String(5.0, 0)` is `"5"`, not `" 5"`: they pad to `decimalPlaces + 2` characters |
| `Print` / `Stream` abstract base classes | ✅ | Added in v0.2.1. Original implementation (not a port of ArduinoCore-avr/API's LGPL 2.1 `Print`/`Stream`), matching the real class hierarchy: any class can inherit `Print` directly (no hardware/pins required) and get every `print()`/`println()` overload for free by implementing just `write(uint8_t)`; `Stream` adds `available()`/`read()`/`peek()` plus the polled `find`/`parseInt`/`readBytes`/etc. helpers. `SerialClass` (`Serial`/`Serial1`) derives from both, and since v0.7.0 so does `TwoWire` (`Wire`). `print()`/`println()` return `size_t` (bytes written), matching real Arduino. Since v0.7.0 `Print` also has a virtual `flush()` that does nothing unless overridden, and `Stream` has the `uint8_t*` forms of `readBytes()`/`readBytesUntil()`, as in ArduinoCore-API |
| `Printable` interface | ✅ | `Print::print`/`println` accept any class implementing `size_t printTo(Print&) const`. Now that `print()`/`println()` return real `size_t` byte counts, the common third-party idiom `size_t n = 0; n += p.print(x); ...; return n;` inside `printTo()` works as written (this previously required a workaround before the `Print`/`Stream` refactor) |
| `Print::setWriteError` / `getWriteError` / `clearWriteError` | ✅ | Matches ArduinoCore-API's placement/signatures (`setWriteError` is `protected`, for a derived class to call internally; `getWriteError`/`clearWriteError` are `public`). Needed by third-party libraries that derive from `Print` and track write failures this way (e.g. the official `SD` library's `SdFile`/`File`) — fixes [#3](https://github.com/teddokano/mcx-arduino-core/issues/3) |
| `PROGMEM` / `pgm_read_byte`/`_word`/`_dword`/`_float`/`_ptr` / `PSTR` | ✅ | No-ops — flash and RAM share one address space on this Cortex-M target, unlike AVR's Harvard split. Declared for sketch/library compatibility only |
| `F("...")` / `__FlashStringHelper` | ✅ | Works with `Serial.print`/`println` and `String` (construct/concat) |

## Compatibility Macros

| API | Status | Notes |
|-----|--------|-------|
| Math constants / compat macros | ✅ | `PI`, `min`/`max`, `bitRead`/`bitWrite`, `map`, etc. (UNO R3/R4 compatible) |
| `word(h, l)` / `makeWord` / `_BV` | ✅ | Added in v0.7.0. `word(...)` is a function-like macro, as on AVR, so `word` still works as a type name |
| `itoa` / `utoa` / `ltoa` / `ultoa` / `dtostrf` | ✅ | Added in v0.7.0, with avr-libc's signatures. `int` and `long` are both 32 bits here, so negative values in a non-decimal base come out 32 bits wide (`itoa(-1, s, 16)` is `"ffffffff"`, not AVR's `"ffff"`). `dtostrf` rounds, and a negative width left-aligns, as on AVR |
| `yield` | ✅ | No-op — no cooperative scheduler on this core |
| Character functions (`isAlpha`, `isDigit`, `isSpace`, etc.) | ✅ | Thin wrappers over `<cctype>` |
| `ARDUINO` version macro | ✅ | Defined as `10819` via `platform.txt`, for libraries that gate on `#if ARDUINO >= 100` etc. |
| `ARDUINO_ARCH_MCX` / `ARDUINO_FRDM_MCXA153` / `ARDUINO_FRDM_MCXN947` | ✅ | Defined via `platform.txt` (the board-specific one derived from `boards.txt`'s `build.board`, so only the macro matching the currently-selected board is defined) |
| `MCX_ARDUINO_CORE_VERSION_MAJOR/_MINOR/_PATCH`, `MCX_ARDUINO_CORE_VERSION`, `MCX_ARDUINO_CORE_VERSION_VAL()`, `MCX_ARDUINO_CORE_VERSION_STR` | ✅ | This package's own release version (unrelated to `ARDUINO`, which is the Arduino API level) — `mcx_arduino_core_version.h`, included from `Arduino.h`. `MCX_ARDUINO_CORE_VERSION` packs major/minor/patch into one integer for `#if MCX_ARDUINO_CORE_VERSION >= MCX_ARDUINO_CORE_VERSION_VAL(0, 4, 0)`-style compile-time gating |
