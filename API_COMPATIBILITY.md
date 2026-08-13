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

## Serial

| API | Status | Notes |
|-----|--------|-------|
| `Serial.begin` / `print` / `println` / `printf` | ✅ | |
| `Serial.read` / `available` / `write` | ✅ | `write` has all 4 standard overloads (`uint8_t`, `const char*`, `(const uint8_t*, size_t)`, `(const char*, size_t)`) |
| `Serial.print`/`println` with `BIN` base | ✅ | Fixed in v0.2.1 — previously silently printed decimal instead of binary |
| `Serial.flush` | ✅ | Blocks until the hardware finishes shifting out the last byte, not just until the software TX buffer is empty |
| `Serial.peek` | ✅ | Only meaningful after `begin()` (always the case for `Serial`/`Serial1`), since it reads the RX ring buffer |
| `Serial.setTimeout` / `readBytes` / `readBytesUntil` / `readString` / `readStringUntil` / `parseInt` / `parseFloat` / `find` | ✅ | Polled, `millis()`-based timeout (default 1000ms) |
| `Serial.find(target, length)` / `findUntil(target, terminator)` | ✅ | Added in v0.2.1 |
| `Serial.availableForWrite` | ✅ | Added in v0.2.1 — free bytes in the TX ring buffer (max 255) |
| `Serial1` | ✅ A153 / ❌ N947 | Hardware UART on `D0`/`D1`, separate from USB-bridged `Serial`. **Not available on FRDM-MCXN947**: on that board `D0`/`D1`'s only UART-capable peripheral is the same FlexComm instance `Wire` uses for I2C, and a FlexComm can only be one peripheral mode at a time -- `Serial1` and `Wire` can't coexist in one sketch there, so it isn't wired up at all (referencing `Serial1` fails to compile) |

## Wire (I2C / I3C)

| API | Status | Notes |
|-----|--------|-------|
| `Wire.begin` / `beginTransmission` / `endTransmission` | ✅ | |
| `Wire.write` / `read` / `requestFrom` / `available` | ✅ | |
| `Wire.setClock` | ✅ | |
| `Wire.end` | ✅ | Added in v0.2.1 — releases the I2C/I3C peripheral |
| `Wire.setWireTimeout` / `clearWireTimeoutFlag` / `getWireTimeoutFlag` | ❌ | Not supported — a real implementation needs a deadline check inside the blocking SDK transfer calls (`LPI2C_MasterStart`/`Send`/`Receive`/`Stop`), not just the Arduino layer; a stub that doesn't actually abort a hung bus would be misleading |
| I2C slave mode (`Wire.begin(address)`, `onReceive`, `onRequest`) | ❌ | Not supported — master mode only. r01lib has no slave-mode I2C/LPI2C driver to build on; would need new low-level driver work, not just an Arduino-layer shim |
| `Wire1` (I3C, I2C mode) | ✅ | On-board P3T1755 temperature sensor (different physical I3C pins per board -- see the board's pin mapping) |

## SPI

| API | Status | Notes |
|-----|--------|-------|
| `SPI.begin` / `end` / `beginTransaction` / `endTransaction` / `transfer` / `transfer16` | ✅ | `bitOrder` in `SPISettings` is now actually applied to hardware (was silently ignored before v0.2.1) |
| `SPI.usingInterrupt` / `notUsingInterrupt` | ✅ | No-op — declared for sketch compatibility only |
| `SPI.setBitOrder` / `setDataMode` / `setClockDivider` | ✅ | Legacy pre-1.6 API; `setClockDivider` divides `SPI`'s peripheral input clock, not `F_CPU` |

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
| `analogWrite` (PWM) | ✅ | FlexPWM0, fixed 1kHz period (not configurable). Dedicated pins named `PWM0`-`PWM5` on A153, `PWM_0`-`PWM_5` (underscore) on N947 -- different name on purpose, since N947's SDK already uses the bare `PWM0` identifier for the FlexPWM peripheral instance itself |
| `analogReference` | ✅ | No-op — this board's ADC reference voltage is fixed in hardware |
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
| `String` class | ✅ | Original implementation (not a WString port); concatenation (including `long long`/`unsigned long long`, and `F("...")`), `substring`/`indexOf`/`replace`, `toInt`/`toFloat`, `getBytes`/`toCharArray`, free `operator+` for all numeric types and `F("...")`, etc. `reserve()` is a no-op (always allocates exact-fit) |
| `Print` / `Stream` abstract base classes | ✅ | Added in v0.2.1. Original implementation (not a port of ArduinoCore-avr/API's LGPL 2.1 `Print`/`Stream`), matching the real class hierarchy: any class can inherit `Print` directly (no hardware/pins required) and get every `print()`/`println()` overload for free by implementing just `write(uint8_t)`; `Stream` adds `available()`/`read()`/`peek()` plus the polled `find`/`parseInt`/`readBytes`/etc. helpers. `SerialClass` (`Serial`/`Serial1`) derives from both. `print()`/`println()` return `size_t` (bytes written), matching real Arduino |
| `Printable` interface | ✅ | `Print::print`/`println` accept any class implementing `size_t printTo(Print&) const`. Now that `print()`/`println()` return real `size_t` byte counts, the common third-party idiom `size_t n = 0; n += p.print(x); ...; return n;` inside `printTo()` works as written (this previously required a workaround before the `Print`/`Stream` refactor) |
| `PROGMEM` / `pgm_read_byte`/`_word`/`_dword`/`_float`/`_ptr` / `PSTR` | ✅ | No-ops — flash and RAM share one address space on this Cortex-M target, unlike AVR's Harvard split. Declared for sketch/library compatibility only |
| `F("...")` / `__FlashStringHelper` | ✅ | Works with `Serial.print`/`println` and `String` (construct/concat) |

## Compatibility Macros

| API | Status | Notes |
|-----|--------|-------|
| Math constants / compat macros | ✅ | `PI`, `min`/`max`, `bitRead`/`bitWrite`, `map`, etc. (UNO R3/R4 compatible) |
| `yield` | ✅ | No-op — no cooperative scheduler on this core |
| Character functions (`isAlpha`, `isDigit`, `isSpace`, etc.) | ✅ | Thin wrappers over `<cctype>` |
| `ARDUINO` version macro | ✅ | Defined as `10819` via `platform.txt`, for libraries that gate on `#if ARDUINO >= 100` etc. |
| `ARDUINO_ARCH_MCX` / `ARDUINO_FRDM_MCXA153` / `ARDUINO_FRDM_MCXN947` | ✅ | Defined via `platform.txt` (the board-specific one derived from `boards.txt`'s `build.board`, so only the macro matching the currently-selected board is defined) |
