/** attachInterrupt(pin, isr, LOW) test
 *
 *  LOW is level-triggered: unlike FALLING (fires once per press), it
 *  should keep firing repeatedly for as long as the pin stays low. Press
 *  and HOLD SW2 for about a second, then release. The count should jump
 *  by a large number while held (not just +1), confirming this isn't
 *  silently aliasing to RISING/FALLING anymore.
 */

#include <Arduino.h>

volatile unsigned long count = 0;

void callback() {
  count = count + 1;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("attachInterrupt LOW mode test");
  Serial.println("Press and HOLD SW2 for ~1 second, then release");

  pinMode(SW2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SW2), callback, LOW);
}

void loop() {
  static unsigned long lastCount = 0;

  if (count != lastCount) {
    Serial.print("count = ");
    Serial.println(count);
    lastCount = count;
  }

  delay(100);
}
