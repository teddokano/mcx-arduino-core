# Advanced: Native I3C via r01lib (`Arduino_incompatible_API`)

`Wire1` (see [TUTORIAL.md](../TUTORIAL.md)) gives you the on-board I3C bus
as a plain I2C-shaped `TwoWire` object — it puts the I3C peripheral into
`I2C_MODE` and never touches anything I3C-specific. That covers most uses
(reading a sensor at a fixed address), but I3C itself is a different, richer
protocol: dynamic addressing (a device gets assigned a runtime address
instead of using a fixed static one), Common Command Codes (CCC) for
broadcast/direct bus management, and In-Band Interrupts (IBI, a target
signaling the controller without a dedicated interrupt pin). None of that
has an equivalent in Arduino's `Wire` API, so there's nothing for a
Wire-shaped wrapper to expose it through.

To use any of it, construct r01lib's `I3C` class directly instead of going
through `Wire1`. This lives in
[`examples/Arduino_incompatible_API/`](../examples/Arduino_incompatible_API)
— "incompatible" meaning it isn't portable to another Arduino core (it's
r01lib's own API, board-specific), not that it's unsupported or unstable.

## Setup

```cpp
#include <Arduino.h>

I3C i3c(I3C_SDA, I3C_SCL);
```

`I3C_SDA`/`I3C_SCL` are safe to use exactly like this, without checking
which board you're on: unlike `D0`-`D19`/`A0`-`A5`/`MB_*`, these two names
are deliberately excluded from `arduino_io.h`'s pin renumbering, so they
always mean the same raw r01lib pin value whether or not `<Arduino.h>` has
been included — one name, one value, both boards.

Constructing an `I3C` this way defaults to native I3C SDR mode
(`I3C::MODE::I3C_MODE`), the mode this whole guide is about. `Wire1`
switches this same peripheral to `I3C::MODE::I2C_MODE` instead via
`mode()`; don't mix the two on the same object in one sketch.

**FRDM-MCXN947 only**: the on-board I3C bus's SDA/SCL pins are the same two
physical pins as `Serial1` on the MikroBus header (`MB_TX`/`MB_RX`) —
whichever one begins last wins the pins. Don't use both in the same sketch
on that board.

## Full example: dynamic addressing and a raw sensor read

[`examples/Arduino_incompatible_API/r01lib_I3C/r01lib_I3C.ino`](../examples/Arduino_incompatible_API/r01lib_I3C/r01lib_I3C.ino)
talks to the on-board P3T1755 temperature sensor over native I3C, assigning
it a dynamic address from its static one:

```cpp
#include <Arduino.h>

I3C i3c(I3C_SDA, I3C_SCL);

constexpr uint8_t static_address  = 0x48;
constexpr uint8_t dynamic_address = 0x08;
uint8_t w_data[] = { 0 };
uint8_t r_data[2];

int main(void) {
  Serial.begin(115200);
  while (!Serial)
    ;

  status_t r1 = i3c.ccc_broadcast(CCC::BROADCAST_RSTDAA, NULL, 0);
  status_t r2 = i3c.ccc_set(CCC::DIRECT_SETDASA, static_address, dynamic_address << 1);
  Serial.printf("RSTDAA status=%ld, SETDASA status=%ld\r\n", (long)r1, (long)r2);

  while (true) {
    status_t rw = i3c.write(dynamic_address, w_data, sizeof(w_data), I3C::NO_STOP);
    status_t rr = i3c.read(dynamic_address, r_data, sizeof(r_data));

    Serial.printf("write status=%ld, read status=%ld, raw=0x%02X%02X, temp=%f\r\n",
                  (long)rw, (long)rr, r_data[0], r_data[1],
                  (((int)r_data[0]) << 8 | r_data[1]) / 256.0);
    wait(1);
  }
}
```

A few things worth calling out:

- **`int main(void)` instead of `setup()`/`loop()`**. This core's `main()`
  is `weak` (the same mechanism the Arduino build itself relies on), so a
  sketch is free to define its own and skip the Arduino sketch convention
  entirely — this example does, since the pattern is a one-time
  setup-then-loop-forever shape anyway.
- **`Serial.printf()`**, not `Serial.print()`/`println()`. This is r01lib's
  own `Serial::printf()` (a real member function, not an Arduino
  `Print`/`Stream` method), used directly since `Serial` here is still the
  same underlying r01lib object either way.
- **`CCC::BROADCAST_RSTDAA`** resets dynamic address assignment on the bus
  (every I3C target loses whatever dynamic address it had); **`CCC::
  DIRECT_SETDASA`** then assigns `dynamic_address` to whichever target is
  currently listening at `static_address`. From here on the sensor answers
  to `dynamic_address`, not `static_address`.
- **`I3C::NO_STOP`** on the `write()` keeps the bus held with a repeated
  start into the following `read()`, rather than releasing it with a full
  STOP in between — the usual register-pointer-then-read pattern. `read()`
  itself defaults to `stop = STOP` (releases the bus).
- **`wait(1)`** is r01lib's own blocking delay (`wait(double
  delayTime_sec)`, in `mcu.h`) — Arduino's `delay()` works too, this is
  just the r01lib-native spelling used throughout this example category.

## The CCC command set

`ccc_broadcast()`/`ccc_set()`/`ccc_get()` take one of `I3C`'s `CCC` enum
values (`i3c.h`) — see the [MIPI I3C
specification](https://www.mipi.org/specifications/i3c-sensor-specification)
for the full semantics of each:

| Command | Value | Purpose |
|---|---|---|
| `BROADCAST_ENEC` | `0x00` | Enable events (IBI/controller-role-request/hot-join) on the bus |
| `BROADCAST_RSTDAA` | `0x06` | Reset dynamic address assignment |
| `BROADCAST_ENTDAA` | `0x07` | Enter dynamic address assignment (the full DAA procedure — see `DAA()`) |
| `DIRECT_ENEC` | `0x80` | Enable events, addressed to one target |
| `DIRECT_DICEC` | `0x81` | Disable events, addressed to one target |
| `DIRECT_SETDASA` | `0x87` | Assign a dynamic address from a target's static address (used above) |
| `DIRECT_SETNEWDA` | `0x88` | Reassign a target's dynamic address |
| `DIRECT_GETPID` | `0x8D` | Read a target's Provisioned ID |
| `DIRECT_GETBCR` | `0x8E` | Read a target's Bus Characteristics Register |
| `DIRECT_GETDCR` | `0x8F` | Read a target's Device Characteristics Register |
| `DIRECT_GETSTATUS` | `0x90` | Read a target's status |

For a bus with more than one I3C target (this example assumes exactly one),
`I3C::DAA()` runs the full dynamic-address-assignment procedure
(`ENTDAA`) and returns however many devices it discovered — see its
declaration in `i3c.h` for the exact signature.

## In-Band Interrupts (IBI)

Not exercised in the example above, but part of the same class:
`check_IBI()` polls for a pending IBI, and `set_IBI_callback(i3c_func_ptr)`
registers a callback invoked when one arrives. If you're building something
that needs a target to signal the controller asynchronously (rather than
being polled), start there.

## See also

- [TUTORIAL.md](../TUTORIAL.md), §2.9 — `Wire1`, the Arduino-compatible
  I2C-mode wrapper around this same peripheral, for the common case
- [advanced_sdk_tuning.md](advanced_sdk_tuning.md) — the same
  "drop below the Arduino layer" idea applied to GPIO instead of I3C
- [mcxpinstate_guide.md](mcxpinstate_guide.md) — useful when debugging pin
  ownership on a sketch that mixes `I3C` objects with Arduino-level
  peripherals
