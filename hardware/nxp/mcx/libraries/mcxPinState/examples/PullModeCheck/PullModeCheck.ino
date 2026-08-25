/** mcxPinState: INPUT_PULLUP / INPUT_PULLDOWN register check
 *
 *  Puts one pin in INPUT_PULLUP and another in INPUT_PULLDOWN, then prints
 *  the pin-ownership table so the actual PCR pull bits can be read back.
 *
 *  Expected: the INPUT_PULLUP pin's line ends in "PU", the INPUT_PULLDOWN
 *  pin's line ends in "PD". If INPUT_PULLDOWN instead shows neither PD
 *  nor PU, that confirms the suspected PORT_SetPinPullUpDown() bug (its
 *  enable/logic parameters land in the PS/PE fields swapped from what its
 *  own doc comment says) -- pinMode(pin, INPUT_PULLDOWN) would be
 *  silently leaving the pin with no pull at all.
 *
 *  No wiring needed -- this only reads back the PORT PCR register PinState
 *  already has access to, not the actual pin voltage.
 */

#include <Arduino.h>
#include <PinState.h>

PinState pins;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  pinMode(D4, INPUT_PULLUP);
  pinMode(D5, INPUT_PULLDOWN);

  Serial.println();
  Serial.println("Expect: D4's pin PU, D5's pin PD");
  pins.print();
}

void loop() {
}
