# Third-Party Notices

mcx-arduino-core is under the MIT License (see [LICENSE](LICENSE)). This file lists the code
from other projects it contains, and the projects whose interfaces or behavior it was written to match.

Paths in this file are from the top of the repository. The Boards Manager package holds only
`hardware/nxp/mcx/`, which is its top folder, and carries a copy of this file and of `LICENSE` there;
the `examples/` folder is in the repository only.

The Arduino compatibility constants, macros, and templates in
`hardware/nxp/mcx/cores/arduino/arduino_api/Arduino.h` (math constants, `min()`/`max()`,
bit-manipulation macros, `map()`, `random()`/`randomSeed()`, and related
definitions) are original reimplementations written to match the interface
and naming of the following reference projects, for sketch compatibility.
No source code was copied from either project.

- ArduinoCore-avr — https://github.com/arduino/ArduinoCore-avr (LGPL 2.1)
- ArduinoCore-API — https://github.com/arduino/ArduinoCore-API (LGPL 2.1)

The same holds for the rest of the Arduino API layer in
`hardware/nxp/mcx/cores/arduino/arduino_api/` and for the bundled
`hardware/nxp/mcx/libraries/EEPROM/`, `hardware/nxp/mcx/libraries/Servo/`,
`hardware/nxp/mcx/libraries/MsTimer2/`, `hardware/nxp/mcx/libraries/TimerOne/` and
`hardware/nxp/mcx/libraries/mcxTicker/`:
they are written from scratch to
the interfaces those projects define, so that sketches and libraries
written for them build and behave the same. That covers names,
signatures, default values and constant values (for example
`Wire.setWireTimeout()` and its defaults, I2C target mode's `onReceive()`/
`onRequest()`, the `SERIAL_8N1` family, `BitOrder`, `SDA`/`SCL`, the
network base classes `Client`/`Server`/`UDP`/`IPAddress`, the AVR EEPROM
library's API, the official Servo library's API, the MsTimer2 and
TimerOne libraries' APIs, and the API of the Ticker in the ESP32 and
ESP8266 Arduino cores, which mcxTicker takes under its own class name),
with behavior checked against their source
and documentation. The UNO R4 core, the official Servo library, MsTimer2
and TimerOne were consulted the same way, and
avr-libc's documentation for `ltoa()`/`ultoa()`/`dtostrf()`, the
string functions declared in `Arduino.h`, and `<avr/pgmspace.h>`
(`cores/arduino/arduino_api/avr/pgmspace.h`). No source code was copied from
any of them.

- ArduinoCore-renesas (UNO R4) — https://github.com/arduino/ArduinoCore-renesas (LGPL 2.1)
- Servo — https://github.com/arduino-libraries/Servo (LGPL 2.1)
- MsTimer2 — https://github.com/PaulStoffregen/MsTimer2 (LGPL 2.1 or later)
- TimerOne — https://github.com/PaulStoffregen/TimerOne (Creative Commons Attribution 3.0 United States)
- Arduino core for the ESP32 (its Ticker library) — https://github.com/espressif/arduino-esp32 (LGPL 2.1)
- Arduino core for the ESP8266 (its Ticker library) — https://github.com/esp8266/Arduino (LGPL 2.1)
- avr-libc — https://github.com/avrdudes/avr-libc (Modified BSD License)

The Linux LinkServer-discovery logic in `hardware/nxp/mcx/tools/upload.sh`
(checking a fixed install path, then versioned `LinkServer_<version>`
install directories, then falling back to `PATH`) is adapted from
ArduinoCore-zephyr's `tools/upload_pyocd_or_linkserver.sh`.

- ArduinoCore-zephyr — https://github.com/arduino/ArduinoCore-zephyr (Apache License 2.0)

The low-level peripheral driver files under
`hardware/nxp/mcx/cores/arduino/sdk/` (shared across chips) and each
board's `hardware/nxp/mcx/variants/<board>/src/` and `include/`
(chip-specific drivers, device startup, CMSIS device headers), plus each board's CMSIS-SVD
peripheral-register descriptor under
`hardware/nxp/mcx/variants/<board>/svd/` (used for Arduino IDE's debugger
peripheral register view), are sourced from the NXP MCUXpresso SDK for the
corresponding chip, unmodified aside from being extracted from the SDK
package, with two exceptions: the HardFault handler in
`hardware/nxp/mcx/cores/arduino/sdk/semihost_hardfault.c` is changed,
where marked in the file, to report a crash on the serial port instead
of stopping silently; and FRDM-MCXC444's
`hardware/nxp/mcx/variants/frdm_mcxc444/src/startup_mcxc444.cpp` has its
flash option byte changed, where marked, so a low NMI pin at reset does
not stop the board, and turns the watchdog (COP) off itself, where
marked, so that a sketch can keep it. Each file retains its own original copyright
header.

- NXP MCUXpresso SDK — https://github.com/nxp-mcuxpresso/mcux-sdk (BSD-3-Clause)

The bundled `hardware/nxp/mcx/libraries/MCXEthernet/` has two parts. Its
own code (`src/MCXEthernet.h`, `src/MCXEthernet.cpp`, `src/Ethernet*.cpp`
and `src/mcx_eth_internal.h`) is written from scratch to
the official Ethernet library's interface: class and function names,
signatures and default values, the `EthernetLinkStatus` and
`EthernetHardwareStatus` names, and the W5x00 socket status codes that
`EthernetClient::status()` returns, checked by building that library's
examples against it. Its way of running lwIP without an RTOS, from a 1 ms
timer interrupt, follows STM32Ethernet's, whose source was read for it. No
source code was copied from either. The steps that seed lwIP's random numbers
from the chip's ELS follow the NXP MCUXpresso SDK's `ELS_PowerDownWakeupInit()`
(`els_pkc`), read for it and written from the register definitions, not
copied. `src/lwipopts.h`, lwIP's settings for the library, started from the
SDK's `lwip_dhcp` example for FRDM-MCXN947, and part of its option settings
are taken from that example's `lwipopts.h`; the file keeps NXP's copyright
notice for them.

- Ethernet — https://github.com/arduino-libraries/Ethernet (its README states LGPL 2.1; its source files carry MIT notices)
- STM32Ethernet — https://github.com/stm32duino/STM32Ethernet (no license file in the repository)

The rest of that library is imported unmodified: lwIP 2.2.1 (`src/lwip/`,
`src/netif/`, `src/compat/` and `src/lwip_src/`, with lwIP's license in
`LICENSE.lwIP`), and, from the network package of the NXP MCUXpresso SDK
2.16.000 for FRDM-MCXN947, the SDK's lwIP port (`src/arch/` and the
`ethernetif`, `enet_ethernetif` and `enet_configchecks` files in
`src/mcx_port/`) and the ENET, PHY (LAN8741) and GPIO adapter driver files
in `src/mcx_port/`. Each file retains its own original copyright header.
The one change is a cast added in four comparisons of signed and unsigned
integers in the SDK's lwIP port (`enet_ethernetif.c`, `enet_ethernetif_lpc.c`
and two in `ethernetif.c`), so that the library builds without warnings with
the IDE's compiler warnings set to "All"; the code does the same as before.

- lwIP — https://savannah.nongnu.org/projects/lwip/ (BSD-3-Clause)
- NXP MCUXpresso SDK — https://github.com/nxp-mcuxpresso/mcux-sdk (BSD-3-Clause)

The example sketch
`examples/Arduino_incompatible_API/r01lib_I3C_demo/r01lib_I3C_demo.ino`
is converted from NXP's P3T1755 I3C demo (`P3T1755_FRDM_MCX_demo_DAA`),
changed where its header lists, and keeps NXP's copyright header. Per-board
versions of that demo are published in NXP's Application Code Hub. The
sketch's `I2C_device` and `TempSensor` files are copied unmodified from
r01lib's device drivers (MIT License, by the author of this package).

- NXP Application Code Hub, dm-i3c-temperature-sensor — https://github.com/nxp-appcodehub/dm-i3c-temperature-sensor (BSD-3-Clause)

`hardware/nxp/mcx/tools/gdb-bridge` (Go, original code, no dependencies
beyond Go's standard library) relays Arduino IDE 2's built-in debugger to NXP
LinkServer's own gdbserver. Its LinkServer-executable discovery logic
reuses the same adapted pattern as `tools/upload.sh`/`upload.bat`
above (see the ArduinoCore-zephyr notice). Its protocol handling was
also written by reading, but not copying code from, two other
projects' source to determine the exact interoperability details
required (an argument format, a log-line format, a regular
expression) — arduino-cli's debug command construction, and Arduino
IDE 2's bundled `cortex-debug` extension's OpenOCD-server launch and
detection logic. No source code was copied from either project; see
the code comments in `tools/gdb-bridge/src/main.go` for exactly which
details were cross-checked and where.

- arduino-cli — https://github.com/arduino/arduino-cli (GNU GPL v3)
- cortex-debug — https://github.com/Marus/cortex-debug (MIT License, Copyright 2017-2022 Marcel Ball)

`hardware/nxp/mcx/tools/mcx-discovery` (Go, original code, no dependencies
beyond Go's standard library) reports the MCU-LINK serial ports with the board's name,
so Arduino IDE 2 and arduino-cli recognize each board. It speaks
Arduino's pluggable discovery protocol, written from arduino-cli's
published specification of that protocol, and gives its ports the same
address, labels and properties as Arduino's builtin `serial-discovery`,
taken from that tool's output. No source code was copied from either.

- Arduino pluggable discovery specification — https://arduino.github.io/arduino-cli/latest/pluggable-discovery-specification/
- serial-discovery — https://github.com/arduino/serial-discovery (GNU GPL v3)

The executables of `hardware/nxp/mcx/tools/gdb-bridge` and
`hardware/nxp/mcx/tools/mcx-discovery` are built with the Go toolchain and
contain parts of Go's standard library and runtime, which are under this license:

- Go — https://go.dev/ (BSD-3-Clause)

```
Copyright 2009 The Go Authors.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

   * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
   * Neither the name of Google LLC nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```
