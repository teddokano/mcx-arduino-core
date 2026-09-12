# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [0.6.0] - 2026-09-12

### Added
- `docs/porting_a_new_board.md` — what it actually takes to add another FRDM-MCX board, written from the FRDM-MCXN947 port. Covers the variant tree and `boards.txt`, the twenty shared files under `cores/arduino/` that grow a branch per board, and the traps that each cost real debugging time the first time round (ALT mux values that position-counting gets wrong, pins that only reach a test point, peripheral clocks set in two places or neither)
- CI now runs static checks for the release-prep mistakes this project has repeatedly made by hand: `platform.txt`'s version fields drifting apart or away from `Doxyfile`, a stray `*/` closing a block comment early, `mcxPinState`'s pin-name table falling out of step with `arduino_io.h`, an unconfirmed CHANGELOG heading, and a missing `package_nxp_mcx_index.json` entry. The `mcxPinState` check covers a gap the library's own `static_assert` cannot reach — it compares the two tables entry by entry, so a reordering (same length, wrong labels) is caught rather than left to a manual diff
- [`mcxRCServo`](https://github.com/teddokano/mcxRCServo) is now bundled as `libraries/mcxRCServo/`, alongside `mcxPinState` — hobby RC servos (`SG90` positional, `FS90R` continuous, with their datasheet pulse timing built in) driven from the `PWM0`-`PWM5` pins, available to sketches straight after installing this core. Like `mcxPinState`, its own repo stays the development home and the bundled copy is a release-time snapshot. Bundling also puts its examples into CI's compile sweep, which matters more than it sounds: the library drives servos through `analogWriteFrequency()` and `analogWrite()`, exactly the pair changed below
- `examples/release_check/01`'s new "peripheral source clocks" section: reads back every default peripheral's actual clock (`CLOCK_Get*Freq()`) on both boards and asserts it against the values this cycle's clock audit worked out by reading `mcu.cpp`/`clock_config.c`/driver source. This closes the gap that let the 0.4.1-era `SPI`/`SPI1` clock bugs (requested 24MHz, delivered ~31kHz) go unnoticed for a full release cycle — a wrong source clock now fails a no-wiring check instead of only showing up on a logic analyzer someone happens to be watching
- `examples/release_check/13`'s pulse-width check for the bundled `mcxRCServo` library: jumpers `PWM0` to `D8` and measures its own output with `pulseIn()` against SG90's datasheet pulse widths (0.5/1.45/2.4ms). No other check here exercises `analogWriteResolution(16)` on a real FlexPWM pin at duty this low (2.5%-12%) — a scaling bug in that path would otherwise compile clean, panic nowhere, and just leave a servo at the wrong angle
- Two more CI hygiene checks, alongside the five already added: `doxygen-freshness` (release-only — compares the commit that last touched `docs/api/` against commits since then touching Doxygen's `INPUT` directories, so a source change that never got a `doxygen Doxyfile` re-run before release is caught instead of shipping stale generated docs, as happened twice already in 0.4.0 and 0.4.1) and `platform-paths` (every push — every file `boards.txt`/`platform.txt` names by path, e.g. `gdb-bridge`'s per-OS binaries or the linker scripts, must both exist and be tracked by git; a file that's present locally but gitignored would silently vanish from the release zip while every local check kept passing, which nearly happened when a global `~/.config/git/ignore` rule swallowed a new `gdb-bridge` `.exe`)
- `mcxpinstate-verified` CI hygiene check (release-only): `mcxPinState`'s `PinState.cpp` carries a `MCXPINSTATE_VERIFIED_AGAINST` constant that must be bumped by hand each time its pin tables are re-checked against `arduino_io.h`; this check fails release if that constant doesn't match `platform.txt`'s version, since nothing else enforced it and the underlying `#warning` alone stays easy to miss in a normal build

### Changed
- `gdb-bridge` no longer needs a binary or a launcher script per board. The LinkServer device string now lives in the OpenOCD config file that `boards.txt` names per board (`tools/gdb-bridge/<board>.cfg`), which both launch paths already hand to the bridge — Arduino IDE 2's cortex-debug as `-f`, `arduino-cli debug` as `--file`. So one `launch.sh` and one binary per OS/arch now serve every board, replacing the two per-board Windows executables (~2.9MB each) and the two per-board launcher scripts. Adding a board is now a new `.cfg` plus two `boards.txt` lines, with nothing to cross-compile. Verified end-to-end on real hardware, both boards, all three platforms (macOS, Windows, Linux)

- `analogWrite()` on a pin FlexPWM cannot reach used to `panic()` inside `PwmOut`'s constructor, killing the sketch with an SOS blink. Neither board routes FlexPWM to any of `D0`-`D13` (N947: none at all; A153: only `D3`/`D7`, on channels `PWM5`/`PWM4` already own), so every classic-Arduino `analogWrite(9, ...)` did this. It now falls back to `digitalWrite()` — LOW below the midpoint of the current `analogWriteResolution()`, HIGH at or above — which is what AVR's own core does for a pin with no timer (UNO R4's core silently does nothing; either beats a dead sketch). `analogWriteFrequency()` keeps failing loudly there — a GPIO has no period to set, and unlike `analogWrite()` it is this core's own extension, so no ported sketch reaches it by accident. That distinction matters to the [`mcxRCServo`](https://github.com/teddokano/mcxRCServo) library, which drives servos through both calls: on a wrong pin it now still stops with a clear message instead of leaving a servo silently unable to move

### Fixed
- `i2c.cpp`'s `LPI2C_MASTER_CLOCK_FREQUENCY` hardcoded FlexComm2's clock frequency for every LPI2C instance's baud-rate calculation, so FRDM-MCXN947's `Wire2` (FlexComm3) computed its baud rate off the wrong number. No observable effect yet — both FlexComm2 and FlexComm3 currently run at the same 12MHz — but it was one clock reassignment away from breaking silently the same way `SPI`/`SPI1` did in 0.4.1 (their FlexComm instances moved from 12MHz to 48MHz and nothing was watching). Replaced with a per-instance lookup that `panic()`s on an instance it doesn't recognize instead of returning another peripheral's plausible-looking number
- `examples/release_check/01`'s new clock-source assertions (added this cycle, see Added above) immediately found two of their own expected values wrong on real hardware, both from auditing `mcu.cpp`/`clock_config.c` without checking whether the peripheral's own driver attaches its clock independently: FRDM-MCXN947's `Serial1` (FlexComm5) measured 12MHz against an expected "missing attach"; FRDM-MCXA153's `Serial1` (LPUART2) measured 12MHz against an expected 96MHz. Both are `Serial.cpp`'s own `s_pinMap[]` attaching the UART's clock at construction time (`kFRO12M_to_FLEXCOMM5`/`kFRO12M_to_LPUART2`), which `clock_config.c` never touches on either board — the checks' expected values were corrected to match, and the general lesson (a missing attach in `mcu.cpp`/`clock_config.c` may just mean the peripheral's own driver attaches it lazily elsewhere) is now in `docs/porting_a_new_board.md`
- Every step in an Arduino IDE 2 debug session printed `warning: (Internal error: pc 0x318 in read in CU, but not in symtab.)` to the Debug Console on every stop — harmless (backtraces still resolved correctly past `main()`) but noisy throughout a session. Cause: `--gc-sections` discards `ResetISR`'s original code, and DWARF 4's flat absolute-address range lists leave a stale `[0x0, ...)` hole that happens to cover address `0x318`; gdb's own consistency check flags the mismatch every time cortex-debug's backtrace needs to resolve that address. Switched `compiler.opt_flags` from `-gdwarf-4` (an unexplained holdover from the original MCUXpresso project's defaults) to `-gdwarf-5` (GCC 14's own default), whose base-address+offset range encoding doesn't produce the same false hole. Verified on real hardware, macOS and Windows
- `SPI::cs_manual_control(false)` muxed the CS pin to a hard-coded ALT2 when handing it to the LPSPI hardware chip-select. ALT2 is right only for the Arduino-header pin set; FRDM-MCXN947's MikroBus SPI is FlexComm6, which reaches P3_23 on ALT3, so that pin set would have handed CS to whatever else sits on ALT3. The constructor now records the ALT it actually used and `cs_manual_control()` restores that one. No behaviour changes today: the constructor is the only caller and always passes `true` (plain GPIO, ALT0), so the hard-coded value was never reached — found by finishing the ALT sweep below rather than by anything failing
- FRDM-MCXA153's `I2C` constructor programmed the same ALT mux value for all four supported pin pairs. Two of them were wrong: `I3C_SDA`/`I3C_SCL` (P0_16/P0_17) and `MB_SDA`/`MB_SCL` (P3_28/P3_27) carry LPI2C0 on ALT2, not ALT3 — ALT3 on the first pair is `LPSPI0_PCS2`/`PCS3`, and on the second nothing is assigned to ALT3 at all. The chain of pin comparisons was only ever a whitelist — every branch empty, panicking on anything unlisted — with the ALT supplied separately as a single `constexpr`. That shape came from r01lib's first commit and applied to every board back then; the per-pair form arrived later, with the boards whose several LPI2C instances forced a value into each branch. A153 has one LPI2C, so nothing ever forced those branches open, and the question of whether one ALT suited all four pairs was never asked. Only the D18/D19 pair is reached through `Wire`, and that one was right, so nothing surfaced it. Found by auditing every ALT value in the codebase against Zephyr's pinctrl headers rather than waiting for a report — FRDM-MCXN947's 26 pins, FRDM-MCXC444's and FRDM-MCXA156's I2C pins all checked out correct

## [0.5.0] - 2026-08-28

### Added
- [`mcxPinState`](https://github.com/teddokano/mcxPinState) is now bundled with this package as `libraries/mcxPinState/`, so it's available to sketches immediately after installing this core — no separate library install needed. Its own GitHub repo remains the primary development location; the bundled copy is a release-time-synced snapshot. Zero-cost unless a sketch actually constructs a `PinState` object
- `examples/release_check/04_mcxpinstate_audit` — a copy of `mcxPinState`'s own `CombinedPeripheralsAudit` example, so the bundled library above has a place in the pre-release hardware sweep too. Edit the original under `libraries/mcxPinState/examples/`, not this copy
- `test_Wire_frequency_accuracy_N947`, `test_Wire1_frequency_accuracy_N947` and `test_Wire2_frequency_accuracy_N947` — regression tests for the I2C clock fixes below. No logic analyzer and no external device needed: each times a run of probe transfers at several `setClock()` rates and checks they scale as asked. They bound the time of a *single* transfer as well as the total, which is what the first version of these missed — one pathological 122ms stall disappears into the sum of 500 otherwise-fast transfers

### Fixed
- `Wire` and `Wire2` (both boards) left the bus un-terminated after an address-phase NAK — the STOP for one transaction only appeared once the next one started. Writing to an address nothing answers on, the NAK is not visible yet at either of the earlier checks, so it surfaces inside `LPI2C_MasterStop()`, whose own error check consumes it and returns before the stop command is ever written. Measured on hardware: 1500 of 1500 probe transfers. Affects any code that probes for devices, an I2C bus scan most obviously
- `Wire` and `Wire2` (both boards) could hang, or run a truncated transaction, when `setClock()` raised the clock after a slower setting had been used — for example the common `setClock(100000)` … `setClock(400000)` sequence. The SDK's baud-rate routine only ever raises the SDA glitch filter, never lowers it, so the value left by the slower rate stayed in place and was far too large for the faster one: on a logic analyzer the address phase was cut short after three SCL pulses and restarted, and before the STOP fix above it hung outright. Now the filter fields are cleared before each rate change, so every `setClock()` is computed from a clean slate
- `Wire1.setClock()` (FRDM-MCXN947) produced unrelated rates instead of the requested one — 100kHz came out at ~2.5MHz and 10kHz at ~1.25MHz, while 400kHz happened to be correct. In I2C mode the rate is reached through a 3-bit divider off the I3C open-drain rate, which `setClock()` left at its I3C value, putting everything below it out of reach; asking for less silently truncated into that divider. The open-drain rate now follows the requested I2C rate, and rates outside the reachable range are clamped rather than turned into something unrelated. Measured after the fix: 400kHz → 357kHz, 100kHz → 104kHz, floor ~56kHz. One pre-existing limitation on this bus is unchanged and now documented in the source: an address-phase NAK emits no STOP, and a read's termination carries a repeated START. Neither is reached by the on-board sensor, which ACKs and terminates cleanly

### Changed
- `test_combined_peripherals` (and its `release_check/21` mirror) now also exercises `SPI1` on FRDM-MCXN947 (previously A153-only) and adds a `Wire2` bus probe on FRDM-MCXN947 in place of `Serial1` (which N947 can't run alongside `Wire1`/I3C — see `variants/frdm_mcxn947/README.md`), closing a gap the sketch's own comment had flagged since v0.4.0
- `examples/release_check/` renumbered so the number says what a check needs rather than just its position in an ad-hoc sequence: `0n` = nothing at all, `1n` = jumper wires only, `2n` = an external library, module, or board. See `examples/release_check/README.md` for the full table (and docs/DEVELOPMENT_LOG.md for the old→new mapping, since this shuffles what several past hardware-verification notes point at)

## [0.4.1] - 2026-08-23

### Fixed
- `SPI.beginTransaction()` could silently fail to change the actual SPI clock. `SPI::frequency()` disabled the LPSPI module and immediately called the SDK's `LPSPI_MasterSetBaudRate()`, but that function's own guard reads the module's enable bit back before the disable is observable across the LPSPI's clock domain, so it saw the module as still enabled and returned without programming the clock divider — confirmed directly: reading the enable bit right after clearing it returns set, the very next read returns clear. A second, compounding defect made this far worse in practice: the divider variable being written back was left uninitialized, and the SDK call's success/failure was never checked, so whenever the guard silently failed, an indeterminate stack value got written as the actual clock divider — different (and wrong) on every build, which made the bug look like it came and went with unrelated code changes. Fixed by seeding the divider from the currently-programmed value instead of leaving it uninitialized, waiting (bounded) for the disable to actually take effect before calling into the SDK, and only writing the divider back if that call reports success. Found and hardware-verified (both boards) via a real application hitting it hard — an SD card library sharing one SPI bus with a display, switching between two different clock speeds every frame ([Issue #4](https://github.com/teddokano/mcx-arduino-core/issues/4))

## [0.4.0] - 2026-08-22

### Added
- Arduino IDE 2 debugger support (both boards): breakpoints, stepping, and variable/register inspection now work from the IDE's built-in debug UI. Under the hood this uses NXP LinkServer's own gdbserver — not OpenOCD, which has no MCX chip support — via a small relay (`tools/gdb-bridge`) that presents itself as `openocd` (the only debug server backend `arduino-cli`/the IDE's bundled cortex-debug extension know how to drive) but actually launches and bridges to LinkServer. Verified on real hardware, both through the IDE's own "Start Debugging" flow and directly via `arduino-cli debug`, on both FRDM-MCXA153 and FRDM-MCXN947
- Generated Doxygen HTML class reference at [`docs/api/`](docs/api/index.html), covering the r01lib driver core and the Arduino-compatible API layer (`Doxyfile` at the repo root, configured to match [r01lib](https://github.com/teddokano/r01lib)'s own)
- CMSIS-SVD peripheral register descriptors for both boards (`variants/<board>/svd/`, from the NXP MCUXpresso SDK), wired up via `debug.svd_file` — Arduino IDE 2's debugger "Cortex Peripherals" panel can now show live peripheral register values by name instead of raw addresses
- `MCX_ARDUINO_CORE_VERSION_MAJOR`/`_MINOR`/`_PATCH`, `MCX_ARDUINO_CORE_VERSION` (packed integer), `MCX_ARDUINO_CORE_VERSION_VAL()`, and `MCX_ARDUINO_CORE_VERSION_STR` — this package's own release version, for `#if`-gating a sketch or library on a minimum core version at compile time (`mcx_arduino_core_version.h`, sourced from `platform.txt`)
- Standard cross-core pin/board macros that were previously missing here: `NUM_DIGITAL_PINS`, `NUM_ANALOG_INPUTS`, `digitalPinHasPWM(pin)`, `PIN_WIRE_SDA`/`PIN_WIRE_SCL`/`PIN_SPI_SS`/`PIN_SPI_MOSI`/`PIN_SPI_MISO`/`PIN_SPI_SCK`, `SERIAL_PORT_MONITOR`/`SERIAL_PORT_HARDWARE`/`SERIAL_PORT_HARDWARE_OPEN` (same convention as ArduinoCore-avr's `pins_arduino.h`/ArduinoCore-samd's `variant.h`)
- `FRDM_MCXA153`/`FRDM_MCXN947` board-identification macros (`boards.txt` `build.board_defines`), alongside the existing `ARDUINO_FRDM_MCXA153`/`ARDUINO_FRDM_MCXN947`
- `analogWriteFrequency(pin, hz)` — a non-standard, Teensy-style extension to change a PWM pin's period (the standard Arduino API has no such call; official cores leave this to timer-specific code). Hardware-verified with a logic analyzer on both boards
- [`examples/release_check/`](examples/release_check) — consolidated pre-release check sketches grouped by wiring configuration, covering nearly the full Arduino-compatible API surface in a handful of flashes instead of one sketch per feature. Groups `01`-`06` need no or minimal jumpers; `09`/`0A`/`0B`/`0C` need an external library, sensor, or a second board and are numbered separately. All groups hardware-verified on both boards
- [`mcxPinState`](https://github.com/teddokano/mcxPinState) — a companion library (separate repo) that lists which pins are currently claimed by which peripheral instance and flags conflicting or unexpectedly-configured PORT muxes, for debugging pin-ownership bugs. This core exposes the hooks it needs (`pin_registry_note()`/`forget()`/`read_pcr()`/`pin_name()`, all weak defaults defined in `cores/arduino/` and overridden only if a sketch actually links `mcxPinState`, so sketches that don't use it pay nothing)
- Advanced guides under [`docs/`](docs/), past the standard Arduino API: [`advanced_sdk_tuning.md`](docs/advanced_sdk_tuning.md) (calling the MCUXpresso SDK directly for raw GPIO speed), [`advanced_r01lib_i3c.md`](docs/advanced_r01lib_i3c.md) (native I3C via r01lib, for functionality `Wire`-shaped APIs can't expose), [`mcxpinstate_guide.md`](docs/mcxpinstate_guide.md)
- [`README.ja.md`](README.ja.md) — Japanese translation of the top-level README

### Changed
- Full source distribution: the prebuilt r01lib/arduino_layer `.a` libraries are gone, replaced by plain source under `cores/arduino/` (shared) and `variants/<board>/src/` (board-specific), built the same way as other Arduino cores (AVR, SAMD, renesas_uno, ...). Arduino IDE's "Go to Definition" now works — jumping into `pinMode()`, `Serial`, or any other core function lands in the actual implementing `.cpp`, not just its header declaration
- `cores/arduino/` is now split into three subdirectories by origin — `sdk/` (NXP MCX SDK driver files), `r01lib/` (the hardware driver core), `arduino_api/` (the Arduino-compatible API layer) — instead of one flat pile of 84 files
- `MCUXpresso_project/` removed — it only ever existed to build the now-gone prebuilt library
- Added Doxygen documentation throughout `cores/arduino/`'s r01lib and Arduino-compatible-API source, and fixed several inaccuracies found in existing r01lib doc comments along the way (mismatched parameter names, read buffers documented as write buffers, missing parameters, one doc that had been copy-pasted from an unrelated method)
- Per-board `_A153`/`_N947` example sketch pairs that were otherwise identical are now single sketches using `#if defined(FRDM_MCXA153)` / `#elif defined(FRDM_MCXN947)`, instead of two near-duplicate files
- FRDM-MCXN947's `analogWrite()` pin macros renamed back from `PWM_0`-`PWM_5` to plain `PWM0`-`PWM5`, matching FRDM-MCXA153 — the original underscored names existed only to dodge a name collision with the MCUXpresso SDK's own `PWM0`/`PWM1` peripheral-instance macros; that's now resolved with a scoped `#undef` instead

### Fixed
- FRDM-MCXA153: `SPI1` (MikroBus SPI, `LPSPI0`) could hang forever on `transfer()`/`transfer16()`. `LPSPI0`'s clock was never attached in this build's `init_mcu()` — a fix from the earlier prebuilt-library era that was dropped during the source-reconciliation work for this release. Found via on-hardware bisection, fixed, and re-verified on real hardware
- `Wire.h` reused `Arduino.h`'s own include guard, so `#include <Wire.h>` on its own (without `<Arduino.h>` already included in the same translation unit — a common pattern in third-party I2C libraries) failed to compile. Given its own guard and turned into a proper thin wrapper, matching the existing `SPI.h`/`arduino_spi.h` pattern; `arduino_i2c.h`/`arduino_spi.h` were also made self-contained (missing standard-library includes) so both now work when included standalone
- Native I3C traffic (a sketch's own `I3C`/`Wire1` object) on FRDM-MCXN947 could NAK every transaction and never bring up SDA/SCL, if the sketch also happened to declare anything that pulled in the global `Serial1` object -- `Serial`'s constructor unconditionally muxed its TX/RX pins immediately, and the global `Serial1` (on N947, the exact same physical pins as `Wire1`'s I3C1) is always constructed at static-init time regardless of whether `begin()` is ever called. Since static initialization order across translation units is unspecified by C++, `Serial1` could construct after the sketch's own I3C object and steal its pins. Fixed by deferring the pin mux from `Serial`'s constructor to `Serial::begin()`
- `Wire1.end()` could crash (BusFault) on FRDM-MCXN947 -- the `_no_hw`-guarded destructor path (added for this exact I3C-as-I2C case on A153 previously) was missing from N947's copy of `i2c.cpp`
- `INPUT_PULLDOWN` didn't actually enable the pin's pull-down resistor -- `PORT_SetPinPullUpDown()`'s `enable`/`logic` parameters were mapped to the PCR's PE/PS fields backwards. `INPUT_PULLUP` happened to still work, since its `enable`/`logic` combination was symmetric and masked the bug; the existing hardware test for this (from v0.2.1) also didn't catch it, since it only checked that a floating, unconnected pin read LOW -- which it does anyway, pull or no pull. Both the driver and that test have been fixed, and the test now actively drives the pin the opposite way and checks the pull pulls it back
- `SPI.transfer(buffer, count)` could overflow its internal 128-byte stack buffer for transfers larger than 128 bytes; now splits into 128-byte chunks internally, invisible to the caller
- `Wire.requestFrom(address, count, stop)`'s `stop` parameter was silently ignored at two separate levels: `TwoWire::requestFrom()` never passed it down to `I2C::read()`, and (once that was fixed) `I2C::read_core()` itself unconditionally issued a STOP condition no matter what was passed, unlike the already-correct `write_core()`
- `I2C::scan(start, last)`'s `start` parameter was ignored (always scanned from address 0), and the 2-argument overload's display loop had an off-by-one that silently dropped the requested `last` address from the printed table
- Both boards: the default `SPI` instance (and on FRDM-MCXN947, also `SPI1`/MikroBus) ran at roughly half the requested clock or less -- their Flexcomm instances were left attached to the 12MHz FRO clock instead of the 48MHz FRO_HF_DIV clock (FRDM-MCXA153 already used the faster one). Found via a third-party display+SPI library rendering dramatically slower on N947 than on A153 with identical code
- `delayMicroseconds()`/`delay()` were systematically about 26-28% slower than requested -- the MCUXpresso SDK's precise DWT-cycle-counter delay path is gated behind `SDK_DELAY_USE_DWT`, which this platform never defined, so every call fell back to a coarse, inaccurate software loop instead
- `BusIn`/`BusOut::config()` used `static_assert(true, ...)` as a stand-in for a runtime pin-count check, which never fires; replaced with this codebase's usual `panic()` pattern for invalid configuration caught at runtime
- Several long-lived internal example sketches (`test_Analog_read_write`, `test_analog_resolution_and_misc`, and by extension the now-merged `test_digitalWrite_analog_pins`) crashed (`panic()`) on FRDM-MCXN947 because they read `analogRead(A0)`, a channel that's unwired on N947 (N947 supports A2-A5, not A0-A3 like A153); switched to a channel valid on both boards
- `test_PROGMEM_F_ARDUINO_macros` always reported its board-identification check as failed on FRDM-MCXN947 -- it only tested for `ARDUINO_FRDM_MCXA153`

## [0.3.2] - 2026-08-20

### Added
- `analogWriteFrequency(pin, hz)` (both boards): sets a PWM pin's frequency, independent of `analogWriteResolution()`'s bit depth. Non-standard extension, not part of the official Arduino API — modeled on Teensy's function of the same name. Duty is preserved as an absolute pulse width across a frequency change, not as a ratio, so call this before `analogWrite()` to set duty at the new rate. `PWM0`-`PWM5` pair up two-to-a-FlexPWM-submodule and share the period register within each pair (`PWM0`/`PWM1`, `PWM2`/`PWM3`, `PWM4`/`PWM5`) — changing one pin's frequency changes its paired pin's frequency too. Verified on real hardware (logic analyzer) on both boards
- `FRDM_MCXA153`/`FRDM_MCXN947` preprocessor defines (both boards): board-identifier macros sketches can branch on directly, alongside the existing chip-part-number macros (`CPU_MCXA153VLH`, `CPU_MCXN947VDF`)

### Fixed
- `examples/Arduino_incompatible_API/r01lib_I3C`: constructing `I3C` with `I3C_SDA`/`I3C_SCL` under `<Arduino.h>` panicked (SOS blink) — those names get renumbered by `arduino_io.h` once Arduino.h is included, so they no longer match the raw r01lib pin values `I3C`'s constructor validates against internally. Fixed by using the raw physical pin names instead, and extended to build on both boards

## [0.3.1] - 2026-08-19

### Added
- `examples/Arduino_compatible_API/test_GPIO_toggle_speed_SDK_API`: an example of calling the MCUXpresso SDK's GPIO driver (`GPIO_PortSet`/`GPIO_PortClear`) directly from a sketch, bypassing `digitalWrite()`'s overhead. Measured on real hardware: 34.23x faster on FRDM-MCXA153 (784.7ns vs. 22.9ns per toggle), 33.32x on FRDM-MCXN947 (488.8ns vs. 14.67ns) — both boards' speed difference between the two paths tracks their clock ratio almost exactly, as expected
- `examples/Arduino_compatible_API/test_Wire1_onboard_sensor_raw`: reads the on-board P3T1755 temperature sensor via `Wire1` using plain register access (`beginTransmission`/`write`/`endTransmission`/`requestFrom`/`read`), no external driver library required

### Changed
- TUTORIAL.md/.ja.md's I2C/`Wire1` section now uses the library-free register-access example above instead of the external `P3T1755` library, so the section is runnable without any extra installs (the library-based example is kept and cross-referenced as an alternative)

### Fixed
- Building a sketch that uses the official `SD` library no longer prints `-Waddress-of-packed-member` warnings (harmless — the warning is about `SD`'s own packed-struct code, not anything in this core — but distracting). Suppressed via `-Wno-address-of-packed-member` in `platform.txt`

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
