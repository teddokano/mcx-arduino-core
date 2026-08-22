# release_check

Consolidated sketches for efficient pre-release hardware verification --
grouped by wiring configuration, so each group needs only one jumper
setup and one flash to cover many features at once. This complements
(doesn't replace) the one-feature-per-sketch examples under
[`Arduino_compatible_API/`](../Arduino_compatible_API), which remain the
reference examples used by [TUTORIAL.md](../../TUTORIAL.md) and are
still the right place to look when pinpointing exactly which feature
broke.

Run in order:

| # | Sketch | Wiring | Judgment |
|---|--------|--------|----------|
| 1 | `01_no_wiring_checks` | none | automatic (reads "ALL OK"/"N FAILED") |
| 2 | `02_no_wiring_manual_observe` | none | manual (watch/listen -- scope, LA, multimeter, ears) |
| 3 | `03_sw2_interrupts` | none (press the on-board SW2 button as prompted) | manual |
| 4 | `04_serial1_and_gpio_loopback` | Serial1 TX/RX loopback jumper (D0-D1 on A153, MikroBus MB_TX-MB_RX on N947) + D2-D3 jumper | automatic |
| 5 | `05_spi_loopback` | D11-D12 jumper + MikroBus MOSI-MISO jumper | automatic |
| 6 | `06_shiftout_pulsein_loopback` | D5-D8, D6-D9, D10-D11, D13-D7 jumpers (4) | automatic |

Not covered here -- these need external hardware/libraries a release
check can't assume are on hand, and stay as individual examples instead:
`test_Wire_P3T1755`, `test_Wire_setClock`,
`test_Wire_end_find_availForWrite_pullmodes`,
`test_combined_peripherals`, `onboard_temperature_sensor` (all need the
external `P3T1755.h` library), `test_Wire_LM75B` (needs a real
LM75-family sensor wired to D18/D19), `test_analogRead_precision_N947`
(needs an external voltage source on A2).

Also intentionally not folded in: `test_GPIO_D0_to_D7` and
`test_analogWrite_duty` (superseded by the more complete `_all_pins`/
`_all_channels` versions already consolidated into #2),
`test_SPI_loopback_with_a_wire` (an older, weaker sanity check with no
pass/fail verdict, superseded by what's in #5), and
`test_PWM_pin_identify` (a "find my pins" reference tool, not a
pass/fail check).
