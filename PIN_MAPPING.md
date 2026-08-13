# Pin Mapping

Arduino pin names are defined in each board's `io.h` (under
`hardware/nxp/mcx/variants/<board>/include/`), which maps each one to its
physical MCU port pin. This file covers both supported boards.

## FRDM-MCXA153

Pin names (`D0`-`D13`, `D18`/`D19`, `A0`-`A5`, `PWM0`-`PWM5`) are defined in
[`hardware/nxp/mcx/variants/frdm_mcxa153/include/io.h`](hardware/nxp/mcx/variants/frdm_mcxa153/include/io.h).

![pins-FRDM-MCXA153](img/pins-FRDM-MCXA153.png)

| Arduino pin | MCU pin | Notes |
|---|---|---|
| `D0` | `P1_4` | `Serial1` RX |
| `D1` | `P1_5` | `Serial1` TX |
| `D2` | `P2_4` | |
| `D3` | `P3_0` | on-board Blue LED (`BLUE`) |
| `D4` | `P2_5` | |
| `D5` | `P3_12` | on-board Red LED (`RED`) |
| `D6` | `P3_13` | on-board Green LED (`GREEN`) |
| `D7` | `P3_1` | |
| `D8` | `P3_15` | |
| `D9` | `P3_14` | |
| `D10` | `P2_6` | `SPI` CS |
| `D11` | `P2_13` | `SPI` MOSI |
| `D12` | `P2_16` | `SPI` MISO |
| `D13` | `P2_12` | `SPI` SCLK |
| `D18` | `P1_8` | `Wire` (I2C) SDA |
| `D19` | `P1_9` | `Wire` (I2C) SCL |
| `A0`-`A3` | `P1_10`, `P1_12`, `P1_13`, `P2_0` | `analogRead` (LPADC), 10bit |
| `A4`, `A5` | `P3_31`, `P3_30` | digital I/O only, not ADC-capable |
| `PWM0`-`PWM5` | `P3_11`...`P3_6` | `analogWrite` (FlexPWM0), see [`test_PWM_pin_identify`](examples/Arduino_compatible_API/test_PWM_pin_identify) |

Other named pins/peripherals:

| Name | MCU pin(s) | Used by |
|---|---|---|
| `USBTX` / `USBRX` | `P0_3` / `P0_2` | `Serial` (USB-bridged UART) |
| `I3C_SDA` / `I3C_SCL` | `P0_16` / `P0_17` | `Wire1` (I3C, I2C mode) — on-board P3T1755 temperature sensor |
| `SW2` / `SW3` | `P3_29` / `P1_7` | on-board push buttons |

> **Note on `Wire1`**: `I3C_SDA`/`I3C_SCL` are wired to the MCU's I3C peripheral, not a
> second I2C controller. `Wire1` drives that peripheral in I2C-compatibility mode, so it
> exposes the same `TwoWire` API as `Wire` and talks to plain I2C devices (such as the
> on-board P3T1755) — no I3C-specific features (dynamic addressing, IBI, higher clock
> rates, etc.) are used or exposed.

## FRDM-MCXN947

Pin names (`D0`-`D13`, `D18`/`D19`, `A0`-`A5`, `PWM_0`-`PWM_5`) are defined in
[`hardware/nxp/mcx/variants/frdm_mcxn947/include/io.h`](hardware/nxp/mcx/variants/frdm_mcxn947/include/io.h).
See that board's own
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

> **Note on `Wire1`**: same I3C-in-I2C-mode design as A153 (see that section's note
> above), but on different physical pins — `I3C_SDA`/`I3C_SCL` here are `MB_RX`/`MB_TX`
> (the MikroBus RX/TX pins), not the same pins as `Wire`.

> **`Serial1` is not available on this board.** `D0`/`D1`'s only UART-capable
> peripheral (FlexComm2) is the same instance `Wire` already uses for I2C — a
> FlexComm can only be one peripheral mode at a time, so the two can't coexist
> in one sketch. Referencing `Serial1` fails to compile.
