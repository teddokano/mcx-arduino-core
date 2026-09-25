# Porting a new FRDM-MCX board

Written from the FRDM-MCXN947 port, which is the only board added since
the core's structure settled. It is meant to be followed in order: the
early steps are cheap and the later ones need hardware, so a mistake
caught early saves a lot.

Most of the effort is *not* in the new `variants/<board>/` directory. It
is in the twenty-odd shared files under `cores/arduino/` that grow one
more branch, and in the handful of traps that cost real debugging time
the first time round. Both are enumerated below.

## Before starting: how much work is this board?

Sibling boards are cheap; a different silicon family is a rewrite.

| Board | Relationship | What that means |
|-------|--------------|-----------------|
| FRDM-MCXA156 | A153's sibling | Same LPADC/FlexPWM/LPI2C/LPSPI/LPUART. Mostly pin tables |
| FRDM-MCXN236 | N947's sibling | Same peripherals, but **no I3C temperature sensor** — an accelerometer instead, so every example built around `Wire1` + P3T1755 needs generalizing |
| FRDM-MCXC444 | Not a sibling | Kinetis: Cortex-M0+, no FPU, ADC16/TPM instead of LPADC/FlexPWM, `fsl_i2c` instead of `fsl_lpi2c`. `analogRead`/`analogWrite`/`tone` are all unimplemented for it. Budget a whole release |

Check this before promising a timeline. `r01lib` already carries a
`CPU_MCXC444VLH` branch in some files, which makes C444 look half-done —
it is not. `AnalogIn.h`/`PwmOut.h` have no C444 branch at all.

## What you need in hand

- **The version-matched MCUXpresso SDK zip** for the board. Not any SDK
  zip: the `fsl_*` drivers must match the ones already vendored here, or
  you get subtle breakage. See "Getting the SDK drivers" below.
- **The board schematic.** `ref/` already holds these for all five FRDM
  boards (`ref/` is gitignored, so it is invisible until you look).
- **Zephyr's pinctrl header for the chip**, e.g.
  `modules/hal/nxp/dts/nxp/mcx/MCXN947VDF-pinctrl.h`. This is generated
  from NXP's own data and is the authority for ALT mux values. See the
  ALT-value trap below for why this matters so much.
- `ref/r01lib_pin_table.xlsx` — per-pin availability.

## 1. Create the variant tree

`variants/<board>/` has four directories:

| Directory | Contents |
|-----------|----------|
| `include/` | CMSIS device header, `*_features.h`, `system_*.h`, `board.h`, `pin_mux.h`, `clock_config.h`, and the SDK driver headers whose content differs per chip. **Only files with no same-named counterpart in `cores/arduino/`** — a duplicate there is dead code, since `platform.txt` puts the core's include paths first |
| `linker/` | One `.ld`. Take the memory map from an MCUXpresso-generated project for the chip and follow the existing scripts' section layout, including the `EEPROM_FLASH` region at the top of flash (see "The `EEPROM` library" below). The HardFault handler (`cores/arduino/sdk/semihost_hardfault.c`) and `start_mcu()` in `mcu.cpp` use `_pvHeapLimit` and `_vStackTop`: the stack's hardware limit is set at the end of the heap, and a fault report close to that limit moves to a fresh stack at the top |
| `src/` | `board/`, `device/`, `startup/`, plus the `fsl_*` drivers that genuinely differ per chip (`fsl_clock`, `fsl_lpadc`, `fsl_lpi2c`, `fsl_lpspi`, `fsl_lpuart`, `fsl_reset`, …) |
| `svd/` | The CMSIS-SVD file, for the IDE's CORTEX PERIPHERALS panel |

Drivers that are byte-identical across boards live in
`cores/arduino/sdk/` instead. Diff before deciding — the set that is
shareable has grown over time.

### Getting the SDK drivers

**Never copy `fsl_*` files from another board's directory.** They look
interchangeable and are not. The correct procedure, established after
this was nearly done for N947:

1. Generate/download the SDK zip for the target board. If you need USB,
   select it in SDK Builder — otherwise the zip contains no USB at all.
2. Confirm the zip matches what is already vendored, by checksumming a
   driver that exists in both and is strongly chip- and version-specific
   (`fsl_clock.c` is the best single indicator).
3. Extract only what you need from that zip.

The SVD file is in the same zip under `devices/<chip>/`, as `.xml`;
rename to `.svd`. Its licence is BSD-3-Clause — confirmed in the zip's
own `SW-Content-Register.txt`, not just the file header.

## 2. Add the board to `boards.txt`

Every property below is required. The ones that bite hardest are marked.

```
<board>.name=FRDM-MCXxxxx (mcx-arduino-core)
<board>.build.mcu=cortex-m33
<board>.build.fpu_flags=            # empty for no-FPU parts
<board>.build.f_cpu=150000000UL     # ! must match the real boot clock
<board>.build.board=FRDM_MCXxxxx    # becomes ARDUINO_FRDM_MCXxxxx
<board>.build.core=arduino
<board>.build.variant=<board>
<board>.build.board_defines=-DCPU_… -DTARGET_… -DFRDM_MCXxxxx
<board>.build.opt_flags=-O2
<board>.upload.maximum_size=…       # ! PROGRAM_FLASH's length, not the chip's flash size
<board>.upload.maximum_data_size=…
<board>.build.ldscript=<chip>.ld
<board>.build.linkserver_target=MCXxxxx:FRDM-MCXxxxx   # ! see below
<board>.upload.tool=linkserver
<board>.vid.0=0x1FC9
<board>.pid.0=0x0143                # ! same for every FRDM board
<board>.debug.server.openocd.path=…/tools/gdb-bridge/launch.sh
<board>.debug.server.openocd.path.windows=…\tools\gdb-bridge\gdb-bridge-windows-amd64.exe
<board>.debug.server.openocd.script=<board>.cfg   # ! carries the LinkServer device
<board>.debug.svd_file=…/variants/<board>/svd/<chip>.svd
```

- `build.linkserver_target` must be a string LinkServer actually knows.
  Check with LinkServer's own device list; do not guess it from the part
  number.
- **VID/PID are identical across all FRDM boards** (same MCU-Link probe
  family), so `arduino-cli board list` cannot tell them apart. Always
  pass `--fqbn` explicitly, and confirm verbally which board is plugged
  in. This has caused at least one round of debugging a board that was
  not the one under test.
- The Windows debug path must use **backslashes throughout**. A path
  mixing `{runtime.platform.path}`'s backslashes with literal forward
  slashes broke the IDE debugger once.

## 3. Add the board's branches to the shared core

This is the bulk of the work, and the part that scales badly. Twenty
files under `cores/arduino/` currently branch per board:

**Pin and instance tables** — the real content of a port.

| File | What to add |
|------|-------------|
| `r01lib/io.h`, `io.cpp` | The pin enum and the `pins[]`/`port_type[]` tables. Everything else keys off these |
| `r01lib/Serial.cpp`, `Serial.h` | `s_pinMap[]` (TX/RX → LPUART instance and per-pin ALT) and the IRQ handlers. The most branch-heavy file |
| `r01lib/i2c.cpp`, `i3c.cpp` | Constructor branches selecting the LPI2C/I3C instance per pin pair |
| `r01lib/r01lib_spi.cpp` | Same, for LPSPI |
| `r01lib/AnalogIn.{h,cpp}`, `PwmOut.{h,cpp}` | **Whole class definitions duplicated per board**, not just tables. Doxygen documents only the first branch |
| `r01lib/irq.{c,h}`, `InterruptIn.cpp` | Interrupt vector wiring |
| `arduino_api/arduino_io.h` | `NUM_ANALOG_INPUTS`, and the renumbering table |

**Clock and symbol differences**

| File | What to add |
|------|-------------|
| `r01lib/mcu.cpp` | `init_mcu()`'s clock setup. **Read the clock trap below before touching this** |
| `arduino_api/arduino_tone.cpp` | Only a divider symbol name — `CLOCK_SetClockDiv`/`kCLOCK_DivCTIMER0` on A153 vs `CLOCK_SetClkDiv`/`kCLOCK_DivCtimer0Clk` on N947. Symbol names drift between chips for no visible reason; expect more of these |

**Peripheral availability**

| File | What to add |
|------|-------------|
| `arduino_api/arduino_i2c.{h,cpp}`, `arduino_serial.{h,cpp}` | Which of `Wire2`/`Serial1` exist at all. On A153 `Wire2` **does not exist as a symbol**, so anything referencing it unconditionally fails to compile |

Work out peripheral availability from the device header before writing
code. A153 has exactly one LPI2C, so an independent `Wire2` is
physically impossible there; its MikroBus I2C pins are just another
route to the same peripheral. Two UART pin pairs may likewise land on
the same FlexComm, which makes them mutually exclusive rather than
independent — that is why N947 has no `Serial1` on D0/D1.

### The `EEPROM` library

The bundled `libraries/EEPROM/` keeps its 1KB in the top of the chip's
on-chip flash, so a new board needs four things before it compiles
against it. Without them, the library stops the build with
`#error EEPROM: this board is not supported`.

1. **An `EEPROM_FLASH` region in the linker script**, at the top of
   flash, taken out of `PROGRAM_FLASH`, plus the two symbols the library
   reads its bounds from:

   ```
   PROGRAM_FLASH (rx) : ORIGIN = 0x0, LENGTH = <flash size - area>
   EEPROM_FLASH (r)   : ORIGIN = <flash size - area>, LENGTH = <area>
   __base_EEPROM_FLASH = <flash size - area>;
   __top_EEPROM_FLASH  = <flash size - area> + <area>;
   ```

   The area is used as two halves, and each half is erased whole, so
   each half must be a whole number of erase sectors
   (`FSL_FEATURE_SYSCON_FLASH_SECTOR_SIZE_BYTES` in the features header;
   8KB on both existing chips). Each half must also hold the 128-byte
   header, the 1KB image and a log of at least a few records. A153 uses
   one 8KB sector per half (16KB); N947 uses 32KB per half (64KB).
   If the chip has two flash banks, put the area in the bank the program
   does not run from, as N947 does. Otherwise, interrupts are held off
   while flash is erased or written, as on A153.
2. **`upload.maximum_size` in `boards.txt` equal to `PROGRAM_FLASH`'s
   length.** If it still says the whole flash, the IDE reports room the
   linker will refuse.
3. **A branch in `EEPROM.cpp`** under `#if defined( CPU_… )`, next to the
   A153 and N947 ones. It gives three things:
   - `UNIT`: the smallest block the flash API programs at any aligned
     offset. On A153 this is a 16-byte phrase. On N947 it is a 128-byte
     page, and `FLASH_Program` returns status 101
     (`kStatus_FLASH_AlignmentError`) at any other alignment. The
     features header gives the page size but not the phrase, so confirm
     `UNIT` by programming at a `UNIT`-aligned offset that is not
     page-aligned.
   - The flash API header to include.
   - The init, erase and program calls in `start()`, `erase_half()` and
     `program()`.

   A153 goes through the boot ROM (`fsl_romapi.h`,
   `FLASH_API->flash_init`), while N947 links `fsl_flash.c`. The same
   family can take either route, so check the SDK for the target chip.
4. **The SDK flash driver files**, from the same SDK zip as the rest
   (see "Getting the SDK drivers"): headers into `variants/<board>/include/`,
   and the `.c` if there is one into `variants/<board>/src/`. They are
   linked only when a sketch uses `EEPROM`, so a mistake here does not
   show up in `hello_world`.

Then, on hardware:
- Run `release_check/06_eeprom` twice: once across its own reset, and
  once after uploading it again.
- Then `release_check/07_eeprom_reset`, 1000 watchdog resets in the
  middle of writes. On FRDM-MCXN947 a reset cut an erase short and left
  flash that faults when read, and every boot after that hung; see
  `read_flash()` in `EEPROM.cpp`. A new chip's flash may behave either
  way, and only this finds out. The WWDT0 setup there has an `#if` per
  board for its clock, which the new board needs too.
- Dump the area before and after an upload and after an IDE Debug
  launch, e.g. with LinkServer's memory read. The two existing chips'
  flash loaders erase only the sectors the program occupies. A new
  chip's loader might mass-erase, and then the data would survive a
  reset but not an upload.

## 4. Update the things outside `hardware/nxp/mcx/`

- **`gdb-bridge`**: add `tools/gdb-bridge/<board>.cfg` carrying one
  `# gdb-bridge-device: MCXxxxx:FRDM-MCXxxxx` line, and point
  `debug.server.openocd.script` at it. **No new binary and no new
  launcher**: the launcher and all five cross-compiled binaries are
  board-agnostic, and gdb-bridge reads the board out of that file,
  which both launch paths already pass it (cortex-debug as `-f`,
  arduino-cli as `--file`). Note that Windows cannot use a `.bat`
  wrapper — the IDE's bundled cortex-debug fails to spawn `.bat` files
  with quoted arguments — which is why boards.txt names the `.exe`
  directly rather than a script. The part of that line before the `:`
  must be the chip name exactly as `LinkServer probes` prints it in its
  `Device` column: with several boards plugged in, that is how
  gdb-bridge picks the probe to debug through. Check it with the new
  board and another one attached, `arduino-cli debug` on each.
- **`mcxPinState`**: `ALIAS_NAMES[]` and `KNOWN_INSTANCES` in
  `PinState.cpp`, in the upstream repo *and* the bundled copy. CI checks
  these against `arduino_io.h`, so a mismatch fails fast.
- **`examples/`**: sketches referencing board-specific pins need a
  `#if defined(FRDM_…)` branch. Board-only sketches take a `_<board>`
  directory suffix, which CI uses to skip them elsewhere.
- **Docs**: `PIN_MAPPING_<board>.md`, `variants/<board>/README.md`,
  the `README.md` board table, `API_COMPATIBILITY.md` differences.

## 5. Traps that cost real time the first round

Every one of these was paid for once already.

**ALT mux values: use Zephyr's pinctrl header, not position counting.**
Counting a signal's position in `pin_mux.c`'s comment list usually gives
the right ALT and is very tempting. It was wrong for exactly two of six
PWM pins on N947, because those pins have an extra entry (`CLKOUT`) that
does not consume a mux slot. The symptom is no output at all on those
pins, after the other four work. Cross-check against the pinctrl header,
then confirm on hardware.

**Verify a pin reaches a connector.** N947's `analogWrite` was first
mapped to P3_6–P3_11, copied from A153. Those are test points on N947 —
correct waveforms come out of the die and nothing is reachable on the
board. Read the schematic page as an **image**: `pdftotext` is not
trustworthy on NXP's two-column schematics, which interleave unrelated
rows.

**A peripheral's clock may be set in two places, or neither.** Both
`mcu.cpp`'s `init_mcu()` and the variant's `clock_config.c` can attach
clocks. `init_mcu()` calls `BOARD_InitBootClocks()` — and therefore
`clock_config.c` — *after* its own setup, so `clock_config.c` wins on
anything it touches. The two existing boards do opposite things:

| | A153 | N947 |
|---|---|---|
| Boot clock | `BOARD_BootClockFRO96M()` | `BOARD_BootClockPLL150M()` |
| `clock_config.c` peripheral attaches | **re-attaches all of them** to FRO_HF_DIV | **touches none** |
| `mcu.cpp`'s attaches | all overridden — **dead code** | the only thing setting them |
| Per-peripheral dividers | `mcu.cpp`'s survive (`clock_config.c` sets none) | same |

So the effective sources are:

| Peripheral | A153 | N947 |
|---|---|---|
| `Wire` | LPI2C0 @ 96MHz | FlexComm2 @ 12MHz |
| `Wire1` | I3C0 @ 48MHz (96/2) | I3C1 @ 25MHz (PLL0 150/6) |
| `Wire2` | — | FlexComm3 @ 12MHz |
| `SPI` | LPSPI1 @ 96MHz | FlexComm1 @ 48MHz |
| `SPI1` | LPSPI0 @ 96MHz | FlexComm6 @ 48MHz |
| `Serial1` | LPUART2 @ 96MHz | FlexComm5 @ **reset default — no attach exists** |

That asymmetry produced the same bug three times on N947 (default `SPI`,
then `SPI1`, then suspected on `Wire2`), each time surfacing as
"requested 24MHz, measured 31kHz". For every peripheral you enable,
check both files and **measure the result** — `CLOCK_Get…ClkFreq()`
printed once over `Serial` is enough to see what you actually got.

Note also that a driver which *queries* its clock
(`CLOCK_GetLpi2cClkFreq()`) survives all of this, while one that assumes
a fixed instance does not. A153's I2C queries; N947's asks for FlexComm2
unconditionally even when the instance in use is FlexComm3.

**Check I2C target mode on each LPI2C, not just the controller side.**
`Wire.begin(address)` rests on the SDK's `LPI2C_Slave*` API, and neither
existing board gave it for free. A153's `fsl_lpi2c` (2.5.4) switches the
master off when it arms the slave, while N947's (2.2.4) doesn't, so
`arduino_i2c.cpp` switches it back on; a board whose SDK is a different
version again may do something else. And on N947, LPI2C3 (`Wire2`)
never responds as a target at all, though it works as a controller and
is set up identically to LPI2C2 (`Wire`), which does; the cause was not
found, so `begin(address)` refuses that bus by its pins. Run
`test_Wire_target_self` (the board as its own target, no wiring) on
every LPI2C the new board exposes, and refuse the ones that fail the same
way.

**Aggregate initialization of SDK config structs is not portable.**
`port_pin_config_t` is a bitfield whose members depend on
`FSL_FEATURE_PORT_HAS_*`. A positional `{a, b, c}` initializer that
compiles on one chip fails on the next. Assign by field name.

**Static initialization order across translation units is unspecified.**
A global peripheral in a sketch and a global `Serial1` in the core can
claim the same pins, and whichever is constructed last wins. This left
N947's I3C silently disconnected from its bus, presenting as "SDA and
SCL just sit high". The core now defers `Serial`'s pin mux to `begin()`,
but any new always-constructed global that grabs pins can reintroduce it.

**SDK macros can collide with pin names.** N947's SDK defines `PWM0`/
`PWM1` as FlexPWM instance pointers, which collides with the pin macros.
`io.h` `#undef`s them; the driver keeps a copy of the SDK meaning saved
before the include. A153 has no such collision, so the pattern only
appears on one board and is easy to miss.

**Watch for `*/` inside block comments.** Writing something like
`ARD_D*/ARD_A*` in a doc comment closes it early and breaks the build
far from the real mistake. This happened four times. CI now checks it.

## 6. Bring-up order

Follow it; each step's tools depend on the previous one working.

1. **Compile only** — `hello_world` for the new FQBN.
2. **GPIO** — blink. Proves clocks, linker script, startup, upload.
3. **`Serial`** — everything after this is easier to diagnose with
   working output. Note that reading it from a sandboxed shell may fail
   for reasons unrelated to the firmware; use a real terminal app.
4. **The rest**, each against a scope or logic analyzer:
   `analogRead` (against a known voltage, not just "it changes"),
   `analogWrite`, `tone`, `Wire`/`Wire1`, `SPI`.
5. **All GPIO pins** — a walking-bit sketch over every named pin. This
   is what caught `pinMode()` failing to reclaim pins the boot code had
   muxed to a peripheral.
6. **`examples/release_check/`** — the full sweep, both boards.

Do not trust a compile-clean sweep as evidence the board works. The
source-distribution migration compiled 114 sketches on both boards and
then hung on the first hardware run.

## See also

- [`variants/frdm_mcxn947/README.md`](../hardware/nxp/mcx/variants/frdm_mcxn947/README.md) — the N947 port's own record, including verified behaviour and known quirks
- [`docs/mcxpinstate_guide.md`](mcxpinstate_guide.md) — the pin-ownership auditor, which exists because of the conflicts described above
- [`PIN_MAPPING_A153.md`](../PIN_MAPPING_A153.md), [`PIN_MAPPING_N947.md`](../PIN_MAPPING_N947.md) — the shape the new board's pin table should take
