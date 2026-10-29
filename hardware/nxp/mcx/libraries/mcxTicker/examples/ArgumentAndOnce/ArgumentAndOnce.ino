/** ArgumentAndOnce: several mcxTickers, a function with an argument, and
 *  once().
 *
 *  One function, toggle(), blinks two LEDs, each at a period of its own:
 *  the pin comes as its argument. After 5 s, a once() stops the red one.
 *  Every second, loop() prints which are still running.
 *
 *  The functions run in the timer interrupt: keep them short. Printing on
 *  Serial is better left to loop(), as here.
 *
 *  Wiring: none (the on-board RGB LED).
 */

#include <mcxTicker.h>

mcxTicker redBlinker;
mcxTicker blueBlinker;
mcxTicker stopper;

void toggle(int pin) {
  digitalWrite(pin, !digitalRead(pin));
}

void stopRed() {
  redBlinker.detach();
  digitalWrite(RED, HIGH);  // off: the LED lights on LOW
}

void setup() {
  Serial.begin(115200);
  pinMode(RED, OUTPUT);
  pinMode(BLUE, OUTPUT);
  digitalWrite(RED, HIGH);
  digitalWrite(BLUE, HIGH);

  redBlinker.attach_ms(150, toggle, RED);  // every 150 ms
  blueBlinker.attach(1.0, toggle, BLUE);   // every second
  stopper.once(5.0, stopRed);              // once, 5 s from now
}

void loop() {
  static unsigned long last = 0;

  if (millis() - last >= 1000) {
    last = millis();
    Serial.print("red: ");
    Serial.print(redBlinker.active() ? "running" : "stopped");
    Serial.print(", blue: ");
    Serial.println(blueBlinker.active() ? "running" : "stopped");
  }
}
