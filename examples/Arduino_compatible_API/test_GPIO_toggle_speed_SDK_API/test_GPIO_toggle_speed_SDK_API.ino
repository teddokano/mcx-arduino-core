/** GPIO toggle-speed test, and an example of calling the MCUXpresso SDK
 *  directly from a sketch.
 *
 *  pinMode() (the Arduino API) does the one-time GPIO setup -- direction
 *  and PORT mux. The actual HIGH/LOW toggling is then done two ways, back
 *  to back, so the toggle rate can be compared:
 *
 *   1. digitalWrite() -- the normal Arduino API
 *   2. GPIO_PortSet()/GPIO_PortClear() -- the underlying NXP MCUXpresso
 *      SDK's GPIO driver (fsl_gpio.h), called directly from the sketch,
 *      bypassing digitalWrite()'s per-call pin resolution
 *
 *  digitalPinToPort()/digitalPinToBitMask() (this core's own fast-GPIO
 *  helpers, meant for exactly this kind of bit-banging use) resolve the
 *  raw GPIO_Type pointer and bitmask that pinMode() already set up, so
 *  the SDK calls know which physical pin to hit -- pinMode() must run
 *  first.
 *
 *  After the timed comparison, loop() keeps toggling via the SDK API
 *  continuously -- probe D2 with a scope/logic analyzer to see the
 *  resulting square wave and its frequency directly.
 *
 *  Both timed loops are manually unrolled (UNROLL iterations' worth of
 *  toggles per loop body) so the measurement reflects the toggle cost
 *  itself rather than the loop's own compare/increment/branch overhead --
 *  that overhead matters little for digitalWrite() (each call is already
 *  much more expensive than a loop iteration), but for the raw SDK path,
 *  where a single toggle is just two inlined register writes, the loop
 *  bookkeeping could otherwise dominate the measurement.
 */

#include <Arduino.h>
#include "fsl_gpio.h"

#define TEST_PIN     D2
#define UNROLL       10UL
#define TOGGLE_COUNT (100000UL * UNROLL)

static GPIO_Type *port;
static uint32_t   mask;

#define DW_TOGGLE  digitalWrite(TEST_PIN, HIGH); digitalWrite(TEST_PIN, LOW);
#define SDK_TOGGLE GPIO_PortSet(port, mask); GPIO_PortClear(port, mask);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  pinMode(TEST_PIN, OUTPUT);  // Arduino API: direction + PORT mux to GPIO

  port = digitalPinToPort(TEST_PIN);
  mask = digitalPinToBitMask(TEST_PIN);

  Serial.println("GPIO toggle speed test on D2");
  Serial.print("TOGGLE_COUNT = ");
  Serial.print(TOGGLE_COUNT);
  Serial.print(" (unrolled ");
  Serial.print(UNROLL);
  Serial.println("x per loop iteration)");
  Serial.println();

  // --- digitalWrite(): the normal Arduino API ---
  uint32_t t0 = micros();
  for (uint32_t i = 0; i < TOGGLE_COUNT / UNROLL; i++) {
    DW_TOGGLE DW_TOGGLE DW_TOGGLE DW_TOGGLE DW_TOGGLE
    DW_TOGGLE DW_TOGGLE DW_TOGGLE DW_TOGGLE DW_TOGGLE
  }
  uint32_t digitalWrite_us = micros() - t0;

  // --- raw MCUXpresso SDK GPIO driver ---
  t0 = micros();
  for (uint32_t i = 0; i < TOGGLE_COUNT / UNROLL; i++) {
    SDK_TOGGLE SDK_TOGGLE SDK_TOGGLE SDK_TOGGLE SDK_TOGGLE
    SDK_TOGGLE SDK_TOGGLE SDK_TOGGLE SDK_TOGGLE SDK_TOGGLE
  }
  uint32_t sdk_us = micros() - t0;

  Serial.print("digitalWrite():  ");
  Serial.print(digitalWrite_us);
  Serial.print(" us total, ");
  Serial.print((double)TOGGLE_COUNT / (double)digitalWrite_us, 3);
  Serial.println(" MHz toggle rate");

  Serial.print("SDK GPIO_Port*:  ");
  Serial.print(sdk_us);
  Serial.print(" us total, ");
  Serial.print((double)TOGGLE_COUNT / (double)sdk_us, 3);
  Serial.println(" MHz toggle rate");

  Serial.print("Speedup: ");
  Serial.print((double)digitalWrite_us / (double)sdk_us, 2);
  Serial.println("x");

  Serial.println();
  Serial.println("Now toggling continuously via the SDK API -- probe D2 to see the square wave.");
}

void loop() {
  GPIO_PortSet(port, mask);
  GPIO_PortClear(port, mask);
}
