# Pin Mapping — FRDM-MCXA153

Arduino pin names are defined in
[`hardware/nxp/mcx/variants/frdm_mcxa153/include/io.h`](hardware/nxp/mcx/variants/frdm_mcxa153/include/io.h),
which maps each one (`D0`-`D13`, `D18`/`D19`, `A0`-`A5`, `PWM0`-`PWM5`) to its
physical MCU port pin.

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

See [PIN_MAPPING_N947.md](PIN_MAPPING_N947.md) for FRDM-MCXN947's pin mapping.
