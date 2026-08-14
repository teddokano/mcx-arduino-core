/** analogRead() precision test for FRDM-MCXN947, using a regulated
 *  voltage source
 *
 *  Feed a known, precise voltage (0 - 3.3V) into A2, and compare the
 *  computed voltage below against the source's actual setting. Reference
 *  is VDDA (nominally 3.3V), resolution is the default 10bit (0-1023).
 *
 *  A2-A5 are all read every cycle so a floating/unconnected channel's
 *  typical noise floor can be compared against the driven one.
 */

#include <Arduino.h>

const float VREF = 3.3f;

void printChannel(const char *name, int pin) {
  int raw = analogRead(pin);
  float volts = raw * VREF / 1023.0f;

  Serial.print(name);
  Serial.print(" raw=");
  Serial.print(raw);
  Serial.print(" volts=");
  Serial.println(volts, 4);
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("analogRead precision test -- feed a known voltage into A2");
}

void loop() {
  printChannel("A2", A2);
  printChannel("A3", A3);
  printChannel("A4", A4);
  printChannel("A5", A5);
  Serial.println("---");

  delay(1000);
}
