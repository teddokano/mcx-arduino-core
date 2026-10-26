/** BlinkEvery150ms: blinks the on-board LED from TimerOne's interrupt, and
 *  lets loop() read how many times it has changed.
 *
 *  Wiring: none.
 *
 *  The count is shared with the interrupt, so loop() copies it with
 *  interrupts off, so that it never reads it halfway through a change.
 */

#include <TimerOne.h>

volatile unsigned long flips = 0;

void flip() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  flips = flips + 1;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  Timer1.initialize(150000);  // 150 ms, in microseconds
  Timer1.attachInterrupt(flip);
}

void loop() {
  noInterrupts();
  unsigned long copy = flips;
  interrupts();

  Serial.print("LED changed ");
  Serial.print(copy);
  Serial.println(" times");
  delay(1000);
}
