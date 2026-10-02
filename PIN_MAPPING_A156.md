# Pin Mapping — FRDM-MCXA156

Arduino pin names are defined in
[`hardware/nxp/mcx/cores/arduino/r01lib/io.h`](hardware/nxp/mcx/cores/arduino/r01lib/io.h),
which maps each one (`D0`-`D13`, `D18`/`D19`, `A0`-`A5`, `PWM0`-`PWM5`) to its
physical MCU port pin.

## Board modification this core assumes

> [!IMPORTANT]
> **Move R59 and R60 to their 2-3 position to use `SPI` on the Arduino header.**
> On a board as shipped they sit at 1-2, which connects header pins `D10`/`D11`
> to `P3_13`/`P3_15` (PWM outputs) instead of `SPI`'s chip select and MOSI.
> This core assumes the modified board.
>
> | Resistor | Header pin | As shipped (1-2) | Modified (2-3), assumed by this core |
> |---|---|---|---|
> | R59 | `D10` | `P3_13` (`PWM1_B2`, also the green LED) | `P2_6` (`LPSPI1_PCS1`) — `SPI` CS |
> | R60 | `D11` | `P3_15` (`PWM1_B1`) | `P2_13` (`LPSPI1_SDO`) — `SPI` MOSI |
>
> `D12` (`P2_16`, MISO) and `D13` (`P2_12`, SCLK) are wired directly and need
> no change. On an unmodified board, `SPI` still drives `P2_6`/`P2_13`, but
> they are not connected to `D10`/`D11`, so CS and MOSI never reach a shield;
> everything else works. R59 and R60 are 0Ω resistors on three-pad
> footprints.

> [!NOTE]
> **Remove R75 and R76 to use `A4`/`A5` as analog inputs.** As shipped, `A4`
> (`P1_12`) and `A5` (`P1_13`) are also wired through R75/R76 to the on-board
> CAN transceiver (TJA1057), whose receive output drives `A4`. The schematic
> says to remove both when the Arduino `A4`/`A5` are used. `A0`-`A3` are not
> affected.

## Arduino header

| Arduino pin | MCU pin | Notes |
|---|---|---|
| `D0` | `P2_11` | `Serial1` RX |
| `D1` | `P2_10` | `Serial1` TX |
| `D2` | `P3_1` | |
| `D3` | `P3_12` | on-board Red LED (`RED`) |
| `D4` | `P3_31` | |
| `D5` | `P3_14` | |
| `D6` | `P3_16` | |
| `D7` | `P1_14` | |
| `D8` | `P1_15` | |
| `D9` | `P3_17` | |
| `D10` | `P2_6` | `SPI` CS — **needs R59 at 2-3** (see above) |
| `D11` | `P2_13` | `SPI` MOSI — **needs R60 at 2-3** (see above) |
| `D12` | `P2_16` | `SPI` MISO |
| `D13` | `P2_12` | `SPI` SCLK |
| `D18` | `P0_16` | `Wire` (I2C) SDA — the on-board P3T1755 is on this bus too, see below |
| `D19` | `P0_17` | `Wire` (I2C) SCL |
| `A0`-`A3` | `P1_10`, `P2_5`, `P2_3`, `P2_4` | `analogRead` (LPADC/ADC1), 10bit |
| `A4`, `A5` | `P1_12`, `P1_13` | `analogRead` (LPADC/ADC1), 10bit — **remove R75/R76 first** (see above) |

All six analog inputs are on one converter (ADC1), unlike FRDM-MCXA153, where
`A4`/`A5` are digital only.

`analogWrite` pins (FlexPWM0, on the motor-control header `J3`; the same pins
and submodules as FRDM-MCXA153):

| Arduino pin | MCU pin | Submodule | Channel |
|---|---|---|---|
| `PWM0` | `P3_11` | sm2 | B |
| `PWM1` | `P3_10` | sm2 | A |
| `PWM2` | `P3_9` | sm1 | B |
| `PWM3` | `P3_8` | sm1 | A |
| `PWM4` | `P3_7` | sm0 | B |
| `PWM5` | `P3_6` | sm0 | A |

> **PWM frequency**: `analogWriteFrequency(pin, hz)` sets a pin's PWM
> frequency (non-standard extension, not part of the official Arduino API —
> modeled on Teensy's function of the same name). `PWM0`/`PWM1`,
> `PWM2`/`PWM3`, and `PWM4`/`PWM5` each pair up on one FlexPWM0 submodule and
> share its period register, so changing one pin's frequency changes its
> paired pin's frequency too.

## Other named pins and peripherals

| Name | MCU pin(s) | Used by |
|---|---|---|
| `USBTX` / `USBRX` | `P0_3` / `P0_2` | `Serial` (USB-bridged UART) |
| `RED` / `GREEN` / `BLUE` | `P3_12` / `P3_13` / `P3_0` | on-board RGB LED (on when LOW). `RED` is also `D3`; `GREEN` reaches the header only on an unmodified board (as `D10`) |
| `SW2` / `SW3` | `P1_7` / `P0_6` | on-board push buttons (`SW2` is WAKEUP, `SW3` is ISP) |

> **The on-board P3T1755 temperature sensor is on `Wire`, not `Wire1`.**
> Unlike FRDM-MCXA153 and FRDM-MCXN947, where it has pins of its own and is
> read through `Wire1`, this board wires it to the Arduino I2C pins
> `D18`/`D19`, with 2.2kΩ pull-ups on the board. It answers at `0x48`, so a
> shield device at `0x48` collides with it. The MCU's I3C peripheral, which
> reaches the sensor only through these same two pins, is not used, and
> `Wire1` is the MikroBus I2C instead (see below). A sketch that reads the
> sensor on several boards picks the instance by board.

## MikroBus header

Plain `digitalWrite`/`digitalRead` GPIO on all of these:

| Name | MCU pin | Notes |
|---|---|---|
| `MB_AN` | `P3_30` | |
| `MB_RST` | `P3_29` | |
| `MB_CS` | `P1_3` | `SPI1` (MikroBus SPI) CS |
| `MB_SCK` | `P1_1` | `SPI1` SCK |
| `MB_MISO` | `P1_2` | `SPI1` MISO |
| `MB_MOSI` | `P1_0` | `SPI1` MOSI |
| `MB_PWM` | `P3_18` | |
| `MB_INT` | `P3_19` | |
| `MB_RX` | `P3_20` | `Serial2` (MikroBus UART) RX |
| `MB_TX` | `P3_21` | `Serial2` TX |
| `MB_SCL` | `P3_27` | `Wire1` (MikroBus I2C) SCL |
| `MB_SDA` | `P3_28` | `Wire1` SDA |

> **`SPI1`** is a plain SPI instance on `MB_MOSI`/`MB_MISO`/`MB_SCK`/`MB_CS`,
> backed by its own peripheral (`LPSPI0`, vs. `SPI`'s `LPSPI1`), so both can
> be used in the same sketch.
>
> **`Wire1`** is a plain I2C instance on `MB_SDA`/`MB_SCL`, backed by its own
> peripheral (`LPI2C3`, vs. `Wire`'s `LPI2C0`), so both can be used in the
> same sketch. The board has 4.7kΩ pull-ups on these two pins. (FRDM-MCXN947
> calls its MikroBus I2C `Wire2`, because its `Wire1` is the on-board
> sensor's bus.)
>
> Unlike FRDM-MCXA153, none of the MikroBus pins is shared with an Arduino
> header pin.
>
> **`Serial2`** is a hardware UART on `MB_RX`/`MB_TX`, backed by its own
> peripheral (`LPUART1`), so `Serial` (USB, `LPUART0`), `Serial1` (`D0`/`D1`,
> `LPUART2`) and `Serial2` can all be used in the same sketch. It is the only
> board with a `Serial2`; FRDM-MCXN947's MikroBus UART is its `Serial1`.

See [PIN_MAPPING_A153.md](PIN_MAPPING_A153.md) and
[PIN_MAPPING_N947.md](PIN_MAPPING_N947.md) for the other boards.
