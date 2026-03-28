# mcx-arduino-core

Arduino board support package for NXP FRDM MCX Series boards.

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

## Supported Arduino APIs

| API | Status | Notes |
|-----|--------|-------|
| `pinMode` | ✅ | |
| `digitalWrite` / `digitalRead` | ✅ | |
| `attachInterrupt` | ✅ | RISING / FALLING / CHANGE |
| `digitalPinToInterrupt` | ✅ | |
| `Serial.begin` / `print` / `println` / `printf` | ✅ | |
| `Serial.read` / `available` / `write` | ✅ | |
| `Wire.begin` / `beginTransmission` / `endTransmission` | ✅ | |
| `Wire.write` / `read` / `requestFrom` / `available` | ✅ | |
| `SPI.begin` / `beginTransaction` / `transfer` / `transfer16` | ✅ | |
| `delay` | ✅ | |
| `analogWrite` (PWM) | n/a | |
| `analogRead` | n/a | |
| `millis` / `micros` | n/a | |
| `tone` / `noTone` | n/a | |
