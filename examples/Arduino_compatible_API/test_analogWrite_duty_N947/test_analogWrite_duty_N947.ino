/** analogWrite() duty-cycle test for FRDM-MCXN947
 *
 *  Drives PWM_0 (physical P2_3, on the "Arduino Shield Compatible Headers"
 *  PWM row -- not one of the D0-D19 header pins) through a fixed sequence
 *  of known duty cycles, holding each for a few seconds so a logic
 *  analyzer / scope / multimeter can measure it. Period is fixed at 1kHz
 *  (this core's analogWrite() doesn't support changing it).
 */

#include <Arduino.h>

struct DutyStep {
  uint8_t value;
  const char *label;
};

DutyStep steps[] = {
  { 0, "0%" },
  { 64, "25%" },
  { 128, "50%" },
  { 191, "75%" },
  { 255, "100%" },
};

const int numSteps = sizeof(steps) / sizeof(steps[0]);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("analogWrite duty-cycle test -- PWM_0 (P2_3), 1kHz");
}

void loop() {
  for (int i = 0; i < numSteps; i++) {
    Serial.print("PWM_0 duty = ");
    Serial.print(steps[i].label);
    Serial.print(" (value ");
    Serial.print(steps[i].value);
    Serial.println(")");

    analogWrite(PWM_0, steps[i].value);
    delay(10);
  }

  Serial.println("--- cycle complete, repeating ---");
}
