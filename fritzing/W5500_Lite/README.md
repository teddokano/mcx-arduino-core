# W5500 Lite: a Fritzing part

A part for [Fritzing](https://fritzing.org/), for drawing the wiring of a small
WIZnet W5500 Ethernet module (the "W5500 Lite": an RJ45 jack and two rows of six
2.54 mm pins) to a board, as in the W5500 row of the
[Ethernet section of API_COMPATIBILITY.md](../../API_COMPATIBILITY.md#ethernet).

To use it, open `W5500_Lite.fzpz` with Fritzing's **File > Import...**; it lands in
the parts bin's **MINE** section.

## Pins

Top view, the jack to the right:

| Upper row, left to right | Lower row, left to right |
|---|---|
| INT | MISO |
| CS | RST |
| SCLK | NC |
| MOSI | VIN (3.3 V) |
| GND | VIN (3.3 V) |
| GND | GND |

The order matches the silkscreen on the module's top side (`MI RST NC V V`
along the lower row). The last pin of the upper row is the W5500's interrupt
output, `INT` (one published pinout shows it cut off, as `NT`).

## Wiring example

The module on FRDM-MCXA153, as `release_check/26` uses it on every board:
3.3V and GND, `D11` (MOSI), `D12` (MISO), `D13` (SCLK), `D10` (CS), `D8` (RST),
and `INT` left unconnected. The module sits turned round on the breadboard, its
jack to the left.

<img src="../../img/W5500-lite_FRDM-MCXA153.png" alt="A W5500 Lite module on a breadboard wired to FRDM-MCXA153: 3V3, GND, D11 MOSI, D12 MISO, D13 SCK, D10 CS, D8 RST, INT left unconnected" width="500">

## Outline: estimated, not measured

No dimensions were at hand. The outline is scaled from a photo of the top using
the pin pitch: the board about 25 x 23 mm, the rows 20.32 mm (8 pitches) apart,
the jack about 28 x 18 mm and sticking out past the right edge by about 5 mm.
The pin positions relative to each other (2.54 mm, two rows of six) don't depend
on that estimate, so the wiring is right; only the drawn outline may be off by a
millimetre or two. To correct it, change the numbers at the top of
`make_part.py` and run

```
python3 make_part.py .
```

which writes the `.fzp`, the four SVGs in `svg/` and `W5500_Lite.fzpz` again.
