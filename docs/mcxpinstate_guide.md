# Advanced: Debugging Pin Ownership with mcxPinState

[mcxPinState](https://github.com/teddokano/mcxPinState) answers a question
this core's own API can't: *right now, which live object actually owns
this pin, and does the hardware agree?* Two peripherals sharing a physical
pin by accident, or a sketch calling `pinMode()` on a pin something else
already claimed, are silent bugs otherwise — everything compiles, and the
symptom shows up as one peripheral mysteriously not working, with no
obvious cause.

It exists because this project's own development kept running into exactly
that class of bug — see mcxPinState's own README for the real ones it
caught along the way (a false pin conflict in `SPI`'s CS handling, a stale
MUX-expectation bug, a genuine `PORT_SetPinPullUpDown()` bug, and I3C's
SDA/SCL briefly reporting the wrong state — all in mcx-arduino-core
itself, all found by pointing this tool at real sketches on real
hardware).

## Installing it

Bundled with this core from v0.5.0 onward: once you've installed
mcx-arduino-core through the Boards Manager, `mcxPinState` is already
available under *Sketch → Include Library* — no separate install step.

[mcxPinState's own GitHub repo](https://github.com/teddokano/mcxPinState)
remains where it's actually developed; the copy shipped here is a snapshot
taken at release time. If you want the latest in-development version
ahead of the next mcx-arduino-core release, or want to file an issue
against the library itself, that's the place — cloning it into your own
sketchbook's `libraries/` folder will shadow the bundled copy.

## Basic usage

```cpp
#include <Arduino.h>
#include <PinState.h>

PinState pins;   // constructing this is what turns the feature on at all

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  pinMode(D2, OUTPUT);
  Wire.begin();

  pins.print();   // dumps both tables to Serial
}

void loop() {
}
```

Constructing a `PinState` object anywhere in the sketch is what actually
enables tracking — without one, the hooks mcx-arduino-core's classes call
on construction/destruction stay empty stubs and cost nothing. There's
nothing to configure beyond that: every `DigitalInOut`-based pin (plain
GPIO, and I2C/I3C/SPI's data pins under the hood), `Serial`/`Serial1`, and
`SPI`/`SPI1`'s pins register themselves automatically.

## Reading the output

`pins.print()` writes two tables to whichever `Print`/`Stream` you pass it
(`Serial` by default).

**Table 1 — every named pin**, whether currently claimed or not:

```
=== Pin MUX state (named pins only) ===
Name(s)                    Pin      MUX  IBE  ODE  Pull  Owner        Status
--------------------------------------------------------------------------------
D2                         P0_29    0    ON   -    -     -            -
D10, SPI_CS, ARD_CS        P0_27    0    ON   -    -     -            -
D18                        P4_0     4    ON   -    -     GPIO         OK
D19                        P4_1     4    ON   -    -     GPIO         OK
...
```

- **Name(s)** — every Arduino-level alias sharing that physical pin,
  comma-joined (D10, `SPI_CS`, and `ARD_CS` are the same pin here).
- **Pin** — the physical pin name (`P0_27`-style), synthesized directly
  from mcx-arduino-core's own data, not a separate lookup table.
- **MUX/IBE/ODE/Pull** — the pin's *live* PORT register state, read back
  right now — not what anyone thinks it should be.
- **Owner** — which class of object currently holds the pin (`GPIO`,
  `Serial`, `SPI`, `AnalogIn`, `PwmOut`), or `-` if nothing does.
- **Status** — `OK`, `-` (unclaimed), `CONFLICT` (more than one live owner
  — see below), or `MISMATCH` (exactly one owner, but the live register
  doesn't match what it asked for — something re-muxed the pin out from
  under it afterward).

**Table 2 — one row per well-known global instance**:

```
=== Peripheral instance state ===
Instance     begun()?  Holds pins                       Status
----------------------------------------------------------------------
Wire         yes       P4_0, P4_1                       OK
Wire1        no        -                                -
SPI          yes       P0_24, P0_26, P0_25              OK
SPI1         no        -                                -
Serial       yes       P0_3, P0_2                       OK
Serial1      no        -                                -
```

This is what tells specific instances apart — Table 1's `Owner` column
only shows the *class* ("GPIO", "SPI"), not which specific global object,
since `Wire`/`Wire1` both register as "GPIO" and `SPI`/`SPI1` both
register as "SPI" under the hood. `begun()?` and `Holds pins` are inferred
from whether that instance's own known pins actually show up correctly
owned in the registry — not from calling any `begin()`-state accessor
(there isn't one), so it's a real answer even for peripherals you didn't
build this library to know about ahead of time.

## Reading a conflict

[`examples/ConflictDemo`](https://github.com/teddokano/mcxPinState/blob/main/examples/ConflictDemo/ConflictDemo.ino)
constructs two `DigitalInOut` objects on D2 at once — a deterministic,
wiring-free way to see what a real conflict looks like:

```
Name(s)  Pin    MUX  IBE  ODE  Pull  Owner        Status
D2       P0_29  0    ON   -    -     GPIO, GPIO   CONFLICT
```

Two owners, same pin, `CONFLICT`. In practice this is what shows up when,
say, a library's `pinMode()` call collides with a peripheral you also
begun on the same physical pin — the exact bug class the on-board I3C bus
sharing pins with `Serial1` on FRDM-MCXN947 (see the [SDK tuning
guide](advanced_sdk_tuning.md) and [r01lib I3C
guide](advanced_r01lib_i3c.md) — the same board-specific gotcha is called
out in both) would produce if you tried it.

## When it's useful outside plain sketches

- **After [dropping to the MCUXpresso SDK directly](advanced_sdk_tuning.md)**
  for GPIO speed — `digitalPinToPort()`/`digitalPinToBitMask()` don't
  register anything new (there's nothing to track beyond what `pinMode()`
  already set up), so a pin driven this way still shows up correctly owned
  by whatever configured its direction/mux.
- **After constructing r01lib objects directly**, like
  [the I3C guide's](advanced_r01lib_i3c.md) `I3C` instance — any
  `DigitalInOut`-based pin registers itself the same way regardless of
  whether it's reached through the Arduino layer or built by hand, so a
  sketch mixing `Wire1` and a raw `I3C` object (don't — see that guide) is
  exactly the kind of conflict this tool would catch immediately.

## See also

- [mcxPinState's own README](https://github.com/teddokano/mcxPinState) —
  full design rationale, the zero-cost-unless-used mechanism, and the real
  bugs found building it
- [advanced_sdk_tuning.md](advanced_sdk_tuning.md)
- [advanced_r01lib_i3c.md](advanced_r01lib_i3c.md)
