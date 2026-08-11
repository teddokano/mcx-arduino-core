# mcx-arduino-core

Arduino board support package for NXP FRDM MCX Series boards.

New here? Start with the [tutorial](TUTORIAL.md) ([日本語版](TUTORIAL.ja.md)).
See [CHANGELOG.md](CHANGELOG.md) for release history.

## Supported Boards

| Board | MCU | Core |
|-------|-----|------|
| FRDM-MCXA153 | MCXA153 (Cortex-M33) | ✅ |
| FRDM-MCXA156 | MCXA156 (Cortex-M33) | 🔜 |
| FRDM-MCXN947 | MCXN947 (Cortex-M33) | 🔜 |
| FRDM-MCXN236 | MCXN236 (Cortex-M33) | 🔜 |

## Requirements

### NXP LinkServer (Required for uploading)

This package uses **NXP LinkServer** for uploading sketches to the board.  
Please install it before using the Upload button in Arduino IDE.

👉 Download: https://www.nxp.com/linkserver

| OS | Installer |
|----|-----------|
| macOS | `.pkg` file, double-click to install |
| Windows | `.exe` installer |
| Linux | `.deb.bin` file |

After installation, the upload script will automatically detect LinkServer — no path configuration needed.

## Installation

1. Open Arduino IDE 2.x
2. Go to **File → Preferences**
3. Add the following URL to **Additional boards manager URLs**:
```
https://raw.githubusercontent.com/teddokano/mcx-arduino-core/main/package_nxp_mcx_index.json
```

4. Go to **Tools → Board → Boards Manager**
5. Search for `NXP MCX` and click **Install**

## Architecture

This package uses a prebuilt library approach:
```
mcx-arduino-core/
├── hardware/nxp/mcx/
│   ├── platform.txt          # Compiler/linker settings
│   ├── boards.txt            # Board definitions
│   ├── cores/arduino/        # Arduino API headers
│   ├── tools/
│   │   └── upload.sh         # Upload script (auto-detects LinkServer)
│   └── variants/
│       └── frdm_mcxa153/
│           ├── include/      # Board-specific headers
│           ├── linker/       # Linker scripts
│           └── lib/          # Prebuilt .a library
└── package_nxp_mcx_index.json
```

The prebuilt library (`lib_r01lib_frdm_mcxa153.a`) contains:
- NXP MCX SDK drivers (fsl_gpio, fsl_lpuart, fsl_lpi2c, fsl_lpspi, ...)
- r01lib core (Serial, I2C, SPI, GPIO, InterruptIn, Ticker, ...)
- Arduino layer (digitalWrite, Wire, SPI, Serial.print, ...)
- Board files (pin_mux, clock_config, board, ...)

## Example Sketch
```cpp
#include "arduino.h"

void setup() {
    Serial.begin(115200);
    Serial.println("Hello from FRDM-MCXA153!");
    pin_mode(RED, OUTPUT);
}

void loop() {
    digitalWrite(RED, LOW);
    delay(500);
    digitalWrite(RED, HIGH);
    delay(500);
}
```

## Building the Prebuilt Library

The prebuilt `.a` library is built with MCUXpresso IDE from the `_r01lib_frdm_mcxa153` project in the [r01lib repository](https://github.com/teddokano/r01lib).

## License

MIT License — see [LICENSE](LICENSE)

## Pin Mapping (FRDM-MCXA153)

Arduino pin names (`D0`-`D13`, `D18`/`D19`, `A0`-`A5`, `PWM0`-`PWM5`) are defined in
[`hardware/nxp/mcx/variants/frdm_mcxa153/include/io.h`](hardware/nxp/mcx/variants/frdm_mcxa153/include/io.h),
which maps each one to its physical MCXA153 port pin.

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

## Supported Arduino APIs

| API | Status | Notes |
|-----|--------|-------|
| `pinMode` | ✅ | |
| `digitalWrite` / `digitalRead` | ✅ | |
| `attachInterrupt` | ✅ | RISING / FALLING / CHANGE |
| `digitalPinToInterrupt` | ✅ | |
| `Serial.begin` / `print` / `println` / `printf` | ✅ | |
| `Serial.read` / `available` / `write` | ✅ | |
| `Serial1` | ✅ | Hardware UART on `D0`/`D1`, separate from USB-bridged `Serial` |
| `Wire.begin` / `beginTransmission` / `endTransmission` | ✅ | |
| `Wire.write` / `read` / `requestFrom` / `available` | ✅ | |
| `Wire1` (I3C, I2C mode) | ✅ | On-board P3T1755 temperature sensor |
| `SPI.begin` / `beginTransaction` / `transfer` / `transfer16` | ✅ | |
| `delay` | ✅ | |
| `analogRead` | ✅ | LPADC, `A0`-`A3`, 10bit (0-1023) |
| `analogWrite` (PWM) | ✅ | FlexPWM0, `PWM0`-`PWM5` only, fixed 1kHz period (not configurable) |
| `millis` / `micros` | ✅ | SysTick(1ms) + DWT cycle counter |
| `tone` / `noTone` | ✅ | CTIMER0, any digital pin, 1 tone at a time |
| `shiftOut` / `shiftIn` | ✅ | Software bit-banged |
| `pulseIn` / `pulseInLong` | ✅ | |
| `random` / `randomSeed` | ✅ | |
| Math constants / compat macros | ✅ | `PI`, `min`/`max`, `bitRead`/`bitWrite`, `map`, etc. (UNO R3/R4 compatible) |
