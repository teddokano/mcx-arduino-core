# release_check

Consolidated sketches for efficient pre-release hardware verification --
grouped by wiring configuration, so each group needs only one jumper
setup and one flash to cover many features at once. This complements
(doesn't replace) the one-feature-per-sketch examples under
[`Arduino_compatible_API/`](../Arduino_compatible_API), which remain the
reference examples used by [TUTORIAL.md](../../TUTORIAL.md) and are
still the right place to look when pinpointing exactly which feature
broke.

Numbering says what a sketch needs, so the groups can be run in order
with one setup each:

- **`0n`** -- nothing at all: no jumpers, no external parts
- **`1n`** -- jumper wires only, still nothing external
- **`2n`** -- an external library, module, or board is required

| # | Sketch | Wiring | Judgment |
|---|--------|--------|----------|
| 01 | `01_no_wiring_checks` | none | automatic (reads "ALL OK"/"N FAILED") |
| 02 | `02_no_wiring_manual_observe` | none | manual (watch/listen -- scope, LA, multimeter, ears) |
| 03 | `03_sw2_interrupts` | none (press the on-board SW2 button as prompted) | manual |
| 04 | `04_mcxpinstate_audit` | none | manual (no `*** CONFLICT ***` / `*** MISMATCH ***` in the tables) |
| 05 | `05_wire2_mikrobus_scan_N947` | none (N947 only -- `Wire2` doesn't exist on A153) | manual (confirm I2C traffic on a logic analyzer) |
| 11 | `11_serial1_and_gpio_loopback` | Serial1 TX/RX loopback jumper (D0-D1 on A153, MikroBus MB_TX-MB_RX on N947) + D2-D3 jumper | automatic |
| 12 | `12_spi_loopback` | D11-D12 jumper + MikroBus MOSI-MISO jumper | automatic |
| 13 | `13_shiftout_pulsein_loopback` | D0-D1, D2-D3, D4-D5, D6-D7 jumpers (4 adjacent pairs) | automatic |
| 21 | `21_combined_peripherals_external_module` | needs the external `P3T1755.h` library + (A153 only) D1-D0 jumper + MikroBus MOSI-MISO jumper | manual (watch the Serial log for WARNING lines) |
| 22 | `22_wire_lm75b_external_module` | needs an external LM75-family sensor module on D18(SDA)/D19(SCL)/3V3/GND | manual (read the printed temperature) |
| 23 | `23_waveshare_tft_touch_external_library` | needs the external `Waveshare_TFT_Touch` library + its LCD/SD hardware (see its own README) | manual (judge the rendered image + draw speed) |

`04_mcxpinstate_audit` is a copy of the bundled `mcxPinState` library's
own `CombinedPeripheralsAudit` example, kept here so a release check
covers the library this package now ships. Edit it in the library
(`hardware/nxp/mcx/libraries/mcxPinState/examples/`), not here.

Not covered here -- these need external hardware/libraries a release
check can't assume are on hand, and stay as individual examples instead:
`test_Wire_P3T1755`, `test_Wire_setClock`,
`test_Wire_end_find_availForWrite_pullmodes`,
`onboard_temperature_sensor` (all need the external `P3T1755.h`
library), `test_analogRead_precision_N947` (needs an external voltage
source on A2).

Also intentionally not folded in: `test_GPIO_D0_to_D7` and
`test_analogWrite_duty` (superseded by the more complete `_all_pins`/
`_all_channels` versions already consolidated into #2),
`test_SPI_loopback_with_a_wire` (an older, weaker sanity check with no
pass/fail verdict, superseded by what's in #12), and
`test_PWM_pin_identify` (a "find my pins" reference tool, not a
pass/fail check).
