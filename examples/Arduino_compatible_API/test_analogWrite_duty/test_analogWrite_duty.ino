/** analogWrite() duty-cycle test
 *
 *  Drives PWM0 (a dedicated PWM pin, not one of the D0-D19 header pins --
 *  see each board's PIN_MAPPING_*.md for its physical location) through a
 *  fixed sequence of known duty cycles, holding each for a few seconds so
 *  a logic analyzer / scope / multimeter can measure it. Period is fixed
 *  at 1kHz (this core's analogWrite() doesn't support changing it).
 *  FRDM-MCXC444 has no PWM0-PWM5 pins, and uses D3 instead.
 */

#include <Arduino.h>

// The pin driven: PWM0, or on FRDM-MCXC444, which has no PWM0-PWM5 pins,
// D3 (TPM1_CH0)
#if defined(FRDM_MCXC444)
#define PWM_PIN D3
#define PWM_PIN_NAME "D3"
#else
#define PWM_PIN PWM0
#define PWM_PIN_NAME "PWM0"
#endif

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

  Serial.println("analogWrite duty-cycle test -- " PWM_PIN_NAME ", 1kHz");
}

void loop() {
  for (int i = 0; i < numSteps; i++) {
    Serial.print(PWM_PIN_NAME " duty = ");
    Serial.print(steps[i].label);
    Serial.print(" (value ");
    Serial.print(steps[i].value);
    Serial.println(")");

    analogWrite(PWM_PIN, steps[i].value);
    delay(10);
  }

  Serial.println("--- cycle complete, repeating ---");
}
