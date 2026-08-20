/** digitalWrite() output test across every D-numbered GPIO pin
 *
 *  Walks a single HIGH pulse through D0..D13, D18, D19 (16 pins -- same
 *  set on both supported boards) one at a time, so a 16-channel logic
 *  analyzer captures the whole set in a single pass and every pin's pulse
 *  is individually distinguishable. The analog pins (A0-A5) are covered
 *  separately by test_digitalWrite_analog_pins.
 *
 *  Note: several of these pins double as SPI/Wire/LED pins elsewhere in
 *  this core (D10-D13=SPI and D18/D19=Wire on both boards; which D-pins
 *  double as the on-board LEDs differs per board -- see each board's
 *  PIN_MAPPING_*.md), but none of those peripherals are begin()'d here,
 *  so this exercises plain digitalWrite()/pinMode(OUTPUT) on the pin
 *  directly.
 */

#include <Arduino.h>

struct PinInfo {
  int pin;
  const char *name;
};

PinInfo pins[] = {
  { D0, "D0" }, { D1, "D1" }, { D2, "D2" }, { D3, "D3" },
  { D4, "D4" }, { D5, "D5" }, { D6, "D6" }, { D7, "D7" },
  { D8, "D8" }, { D9, "D9" }, { D10, "D10" }, { D11, "D11" },
  { D12, "D12" }, { D13, "D13" }, { D18, "D18" }, { D19, "D19" },
};

const int numPins = sizeof(pins) / sizeof(pins[0]);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("digitalWrite all-pin walking-bit test (D0-D13, D18, D19)");

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
