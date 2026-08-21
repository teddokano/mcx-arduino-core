# Advanced: Calling the MCUXpresso SDK Directly

The Arduino API on this core is a thin layer over
[r01lib](https://github.com/teddokano/r01lib), which itself sits on top of
NXP's MCUXpresso SDK. Nothing stops a sketch from reaching straight through
to that SDK layer when `digitalWrite()`'s per-call overhead (pin resolution,
bounds checking, the general-purpose code path) is actually the bottleneck.
This is an escape hatch for the small minority of cases where it matters —
tight bit-banged protocols, high-frequency toggling, ISR-adjacent code — not
a general recommendation. `digitalWrite()` is fine for almost everything;
reach for this only once you've actually measured a problem it causes.

## The pattern

1. Configure the pin the normal way, once, with `pinMode()`. This is where
   direction and PORT mux get set — the part you still want the Arduino API
   to handle.
2. Resolve the pin to raw SDK types with this core's own fast-GPIO helpers,
   `digitalPinToPort()`/`digitalPinToBitMask()` (`GPIO_Type*` and a bitmask)
   — these read back what `pinMode()` already set up, so `pinMode()` must
   run first.
3. From then on, call the MCUXpresso SDK's GPIO driver directly —
   `GPIO_PortSet()`/`GPIO_PortClear()` (`fsl_gpio.h`) — instead of
   `digitalWrite()`.

```cpp
#include <Arduino.h>
#include "fsl_gpio.h"

#define TEST_PIN D2

static GPIO_Type *port;
static uint32_t   mask;

void setup() {
  pinMode(TEST_PIN, OUTPUT);           // Arduino API: direction + PORT mux

  port = digitalPinToPort(TEST_PIN);
  mask = digitalPinToBitMask(TEST_PIN);
}

void loop() {
  GPIO_PortSet(port, mask);            // SDK API: raw register write
  GPIO_PortClear(port, mask);
}
```

SDK headers are already on the include path (`compiler.includes` in
`platform.txt` covers `cores/arduino/sdk/`), so `#include "fsl_gpio.h"` just
works — no extra setup needed.

## Measured cost

[`examples/Arduino_compatible_API/test_GPIO_toggle_speed_SDK_API/`](../examples/Arduino_compatible_API/test_GPIO_toggle_speed_SDK_API/test_GPIO_toggle_speed_SDK_API.ino)
runs both paths back to back on the same pin and times them with `micros()`,
then keeps toggling via the SDK path continuously so the result is also
visible as a square wave on a scope/logic analyzer. Measured on real
hardware, both boards:

| | FRDM-MCXA153 (96 MHz) | FRDM-MCXN947 (150 MHz) |
|---|---|---|
| `digitalWrite()` | 784.7 ns/toggle (1.274 MHz) | 488.8 ns/toggle (2.045 MHz) |
| SDK (`GPIO_PortSet`/`Clear`) | 22.9 ns/toggle (43.620 MHz) | 14.67 ns/toggle (68.166 MHz) |
| Speedup | 34.23x | 33.32x |

The two boards' ratio (1.61x / 1.56x) tracks their clock ratio
(150/96 = 1.5625x) closely for both paths — a sign the comparison is
measuring real toggle cost, not some clock-independent fixed overhead.

Both timed loops in the example are manually unrolled (10 toggles per loop
body) before being timed. That matters more than it might look: for
`digitalWrite()`, each call is already so much more expensive than a loop
iteration's own compare/increment/branch that the unrolling barely changes
the result — but for the raw SDK path, where a single toggle is just two
inlined register writes, the *loop bookkeeping itself* would otherwise
dominate the measurement and understate the real speedup. If you write your
own microbenchmark against a tight instruction sequence like this, unroll it
first.

## What you give up

This bypasses everything the Arduino/r01lib layers do beyond direction and
mux setup — no pin-ownership bookkeeping, no bounds checking, no
portability to another Arduino core. It's also invisible to
[mcxPinState](mcxpinstate_guide.md): `digitalPinToPort()`/
`digitalPinToBitMask()` don't register anything, so a pin driven this way
still shows up correctly as owned by whatever `pinMode()` call configured
it, but nothing about the raw `GPIO_PortSet()`/`GPIO_PortClear()` calls
themselves is tracked (there's nothing to track — direction/mux didn't
change).

The same pattern generalizes past plain GPIO: any r01lib class is fair game
to call into directly once you have an instance (see
[the r01lib I3C guide](advanced_r01lib_i3c.md) for a worked example that
uses r01lib classes directly, not just SDK functions layered under
Arduino's own pins).

## See also

- [TUTORIAL.md](../TUTORIAL.md) — the standard Arduino API walkthrough
- [advanced_r01lib_i3c.md](advanced_r01lib_i3c.md) — using r01lib classes
  directly for functionality the Arduino API layer doesn't expose at all
- [mcxpinstate_guide.md](mcxpinstate_guide.md) — checking what's actually
  configured on a pin, useful when debugging code that mixes Arduino-level
  and SDK-level pin access
