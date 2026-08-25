/** mcxPinState: multi-peripheral pin ownership dump
 *
 *  Brings up several peripherals at once (GPIO, Wire, SPI) and prints the
 *  resulting two-table pin/instance dump. Every claimed pin's Status
 *  column should read "OK", and Wire/SPI should each show begun()=yes in
 *  the second table -- a "CONFLICT" anywhere means two live objects are
 *  fighting over the same physical pin (the bug class this library exists
 *  to catch; see mcx-arduino-core's own session history for a real
 *  example: Serial1 and a sketch-declared I3C object silently re-muxing
 *  the same MikroBus pins on FRDM-MCXN947).
 */

#include <Arduino.h>
#include <PinState.h>

PinState pins;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  pinMode(D2, OUTPUT);
  pinMode(D3, INPUT);

  Wire.begin();

  pinMode(SS, OUTPUT);
  digitalWrite(SS, HIGH);
  SPI.begin();

  Serial.println();
  pins.print();
}

void loop() {
}
