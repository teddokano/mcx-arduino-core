/** Blink: the on-board LED changes every 0.5 s, from an mcxTicker.
 *
 *  loop() is empty: the function runs from the timer interrupt, so it
 *  keeps time whatever loop() does.
 *
 *  Wiring: none.
 */

#include <mcxTicker.h>

mcxTicker blinker;

void toggle() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  blinker.attach(0.5, toggle);  // every 0.5 s
}

void loop() {
}
