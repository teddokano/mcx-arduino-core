/** BlinkEvery500ms: blinks the on-board LED from MsTimer2, while loop()
 *  prints on Serial.
 *
 *  Wiring: none.
 *
 *  The LED changes every 500 ms from the timer interrupt, whatever loop()
 *  is doing, so its slow prints don't hold up the blinking.
 */

#include <MsTimer2.h>

volatile unsigned long flips = 0;

void flip() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  flips = flips + 1;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  MsTimer2::set(500, flip);  // every 500 ms
  MsTimer2::start();
}

void loop() {
  Serial.print("LED changed ");
  Serial.print(flips);
  Serial.println(" times");
  delay(2000);
}
