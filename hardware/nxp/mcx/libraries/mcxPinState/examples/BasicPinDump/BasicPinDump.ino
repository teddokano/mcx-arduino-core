/** mcxPinState basic usage
 *
 *  Constructs a PinState instance (this alone is what turns on pin-
 *  ownership tracking -- see PinState.h), claims a few pins via plain
 *  pinMode(), then prints the ownership table.
 */

#include <Arduino.h>
#include <PinState.h>

PinState pins;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D2, INPUT);

  pins.print();
}

void loop() {
}
