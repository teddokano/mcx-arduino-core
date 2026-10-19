# Fritzing part: FRDM-MCXA153

A [Fritzing](https://fritzing.org/) part for the NXP FRDM-MCXA153 board, with breadboard, schematic and
PCB views. It is not part of the board package: the Arduino IDE never sees this directory.

## What the part has

54 connectors, named as in this core (`D13/SCK`, `A0`, `PWM0`, `MB_SDA`, ...):

- the Arduino sockets J1-J4: `D0`-`D13`, `D18`/`D19`, `AREF`, `A0`-`A5`, `IOREF`, `RESET`, `3V3`, `5V`, `VIN`, `GND` and `NC`
- `PWM0`-`PWM5`, on the inner column of J3
- the mikroBUS sockets J5/J6 (`MB_AN`, `MB_RST`, `MB_CS`, ..., `MB_SDA`)

The mikroBUS pins that share an MCU pin with an Arduino pin (`MB_AN` = `A5`, `MB_RST` = `D7`, `MB_PWM` = `D5`,
`MB_INT` = `D4`, `MB_RX` = `D9`, `MB_TX` = `D8`) are joined as buses, and so are all the `GND`, `3V3` and `5V` pins.
Pin numbers come from the board schematic (SCH-90829) and [PIN_MAPPING_A153.md](../PIN_MAPPING_A153.md); the
positions of the mikroBUS pins were taken from a photo of the board, so they can be a few millimetres off.

## Using it

Drag `FRDM-MCXA153.fzpz` onto a Fritzing window (tested with Fritzing 1.0.1 on macOS), or use Part → Import
if your version has it. Then search the Parts panel for `MCXA153`. Importing the same part twice fails with
"There is already a part with id ...": delete the earlier copy first.

If importing fails, `install_to_fritzing.sh` copies the part into `~/Documents/Fritzing/parts/` instead
(quit Fritzing first, and start it again afterwards).

## Rebuilding

`python3 generate_frdm_mcxa153.py` writes `FRDM-MCXA153/` and `FRDM-MCXA153.fzpz`. Edit the script, not the
generated files. Two details Fritzing insists on: the `.fzp` inside the bundle must be named `part.*.fzp`, and the
part id must be a hex string; a plainly named `.fzp` is rejected with "Unable to load part".
