/** digitalWrite() output test on the analog pins
 *
 *  The analog pins double as plain digital I/O (same DigitalInOut
 *  mechanism as D0-D19), independent of their analogRead() capability.
 *  Walks a single HIGH pulse through each one.
 *
 *  A0/A1 are DISABLED_PIN (not real, wired pins) on FRDM-MCXN947, so only
 *  A2-A5 are walked there; FRDM-MCXA153 has all six wired.
 */

#include <Arduino.h>

struct PinInfo {
  int pin;
  const char *name;
};

PinInfo pins[] = {
#if defined(FRDM_MCXA153)
  { A0, "A0" }, { A1, "A1" },
#endif
  { A2, "A2" }, { A3, "A3" }, { A4, "A4" }, { A5, "A5" },
};

const int numPins = sizeof(pins) / sizeof(pins[0]);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("digitalWrite analog-pin walking-bit test");

  for (int i = 0; i < numPins; i++) {
    pinMode(pins[i].pin, OUTPUT);
    digitalWrite(pins[i].pin, LOW);
  }
}

void loop() {
  for (int i = 0; i < numPins; i++) {
    Serial.print("HIGH: ");
    Serial.println(pins[i].name);

    digitalWrite(pins[i].pin, HIGH);
    delay(200);
    digitalWrite(pins[i].pin, LOW);
    delay(50);
  }

  Serial.println("--- cycle complete, repeating ---");
}
