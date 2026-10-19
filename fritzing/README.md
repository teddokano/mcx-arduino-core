# Fritzing parts for the FRDM boards

[Fritzing](https://fritzing.org/) parts, with breadboard, schematic and PCB views, for the five boards this core
supports. They are not part of the board package: the Arduino IDE never sees this directory.

| Part | Board | Connectors |
|---|---|---|
| `FRDM-MCXA153.fzpz` | FRDM-MCXA153 | 54 |
| `FRDM-MCXA156.fzpz` | FRDM-MCXA156 | 54 |
| `FRDM-MCXN236.fzpz` | FRDM-MCXN236 | 54 |
| `FRDM-MCXN947.fzpz` | FRDM-MCXN947 | 52 (`A0`/`A1` are not usable on this board) |
| `FRDM-MCXC444.fzpz` | FRDM-MCXC444 | 48 (no `PWM0`-`PWM5` pins) |

## What a part has

Connectors named as in this core (`D13/SCK`, `A0`, `PWM0`, `MB_SDA`, ...), each with the MCU pin and the notes
from the board's `PIN_MAPPING_*.md` as its tooltip:

- the Arduino sockets J1-J4: `D0`-`D13`, `D18`/`D19`, `AREF`, `A0`-`A5`, `IOREF`, `RESET`, `3V3`, `5V`, `VIN`, `GND` and `NC`
- `PWM0`-`PWM5`, on the inner column of J3 (not on the FRDM-MCXC444)
- the mikroBUS sockets J5/J6 (`MB_AN`, `MB_RST`, `MB_CS`, ..., `MB_SDA`)

All `GND`, `3V3` and `5V` pins are joined as buses. So are the mikroBUS pins that are the same MCU pin as an Arduino
pin (for example `MB_SCK` and `D13` on the FRDM-MCXN236; see each board's PIN_MAPPING).

The pin names, MCU pins and the bus joins come from the documentation and the board schematics. The board outline,
socket positions, buttons and USB connectors were measured from the photos in `img/` (about 1 mm), so the parts are
for wiring up a breadboard, not for designing a shield PCB.

## Using a part

Drag the `.fzpz` onto a Fritzing window (tested with Fritzing 1.0.1 on macOS), then search the Parts panel for the
board name, for example `MCXA153`. Importing the same part twice fails with "There is already a part with id ...":
delete the earlier copy first.

## Rebuilding

`python3 generate_fritzing_parts.py [A153 A156 N236 N947 C444]` writes one directory and one `.fzpz` per board.
Edit the script, not the generated files. Two details Fritzing insists on: the `.fzp` inside the bundle must be named
`part.*.fzp`, and the part id must be a hex string; a plainly named `.fzp` is rejected with "Unable to load part".
