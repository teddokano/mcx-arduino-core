# Pin Mapping — FRDM-MCXN947

Arduino pin names are defined in
[`hardware/nxp/mcx/variants/frdm_mcxn947/include/io.h`](hardware/nxp/mcx/variants/frdm_mcxn947/include/io.h),
which maps each one (`D0`-`D13`, `D18`/`D19`, `A0`-`A5`, `PWM_0`-`PWM_5`) to its
physical MCU port pin. See that board's own
[`variants/frdm_mcxn947/README.md`](hardware/nxp/mcx/variants/frdm_mcxn947/README.md)
for the full verification status and per-feature notes this table summarizes.

| Arduino pin | MCU pin | Notes |
|---|---|---|
| `D0` | `P4_3` | not `Serial1` — see the `Serial1` note below |
| `D1` | `P4_2` | not `Serial1` — see the `Serial1` note below |
| `D2` | `P0_29` | |
| `D3` | `P1_23` | |
| `D4` | `P0_30` | |
| `D5` | `P1_21` | |
| `D6` | `P1_2` | on-board Blue LED (`BLUE`) |
| `D7` | `P0_31` | |
| `D8` | `P0_28` | |
| `D9` | `P0_10` | on-board Red LED (`RED`) |
| `D10` | `P0_27` | on-board Green LED (`GREEN`); also `SPI` CS |
| `D11` | `P0_24` | `SPI` MOSI |
| `D12` | `P0_26` | `SPI` MISO |
| `D13` | `P0_25` | `SPI` SCLK |
| `D18` | `P4_0` | `Wire` (I2C) SDA |
| `D19` | `P4_1` | `Wire` (I2C) SCL |
| `A0`, `A1` | — | **not available** — not wired to an ADC channel on this board |
| `A2`-`A5` | `P0_14`, `P0_22`, `P0_15`, `P0_23` | `analogRead` (LPADC/ADC0), 10bit |
| `PWM_0`-`PWM_5` | `P3_11`...`P3_6` | `analogWrite` (PWM0/FlexPWM). Named `PWM_0`-`PWM_5` (underscore), not `PWM0`-`PWM5` — this chip's SDK already uses the bare `PWM0` identifier for the FlexPWM peripheral instance itself |

Other named pins/peripherals:

| Name | MCU pin(s) | Used by |
|---|---|---|
| `USBTX` / `USBRX` | `P1_9` / `P1_8` | `Serial` (USB-bridged UART) |
| `I3C_SDA` / `I3C_SCL` (`MB_RX` / `MB_TX`) | `P1_16` / `P1_17` | `Wire1` (I3C, I2C mode) — on-board P3T1755 temperature sensor |
| `SW2` / `SW3` | `A5` (`P0_23`) / `P0_6` | on-board push buttons — note `SW2` shares its pin with `A5` |

> **Note on `Wire1`**: same I3C-in-I2C-mode design as A153 (see
> [PIN_MAPPING_A153.md](PIN_MAPPING_A153.md)'s note), but on different physical pins —
> `I3C_SDA`/`I3C_SCL` here are `MB_RX`/`MB_TX` (the MikroBus RX/TX pins), not the same
> pins as `Wire`.

> **`Serial1` is not available on this board.** `D0`/`D1`'s only UART-capable
> peripheral (FlexComm2) is the same instance `Wire` already uses for I2C — a
> FlexComm can only be one peripheral mode at a time, so the two can't coexist
> in one sketch. Referencing `Serial1` fails to compile.

See [PIN_MAPPING_A153.md](PIN_MAPPING_A153.md) for FRDM-MCXA153's pin mapping.
