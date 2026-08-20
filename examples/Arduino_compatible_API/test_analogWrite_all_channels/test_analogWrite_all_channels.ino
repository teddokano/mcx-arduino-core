/** analogWrite() all-channel + independence test
 *
 *  Drives all six PWM0..PWM5 pins (see each board's PIN_MAPPING_*.md for
 *  their physical location) and checks two things a logic analyzer probed
 *  on all six lines at once can verify:
 *
 *  Phase A: all six channels set to distinct, fixed duty cycles
 *  simultaneously -- confirms every channel actually outputs (not just
 *  PWM0) and that the six values are individually correct.
 *
 *  Phase B: PWM0/PWM1, PWM2/PWM3, and PWM4/PWM5 each share a FlexPWM
 *  submodule (period is shared per submodule -- see PwmOut.h), but duty is
 *  per-channel (A vs B). This phase holds one channel of each pair at a
 *  fixed 50% while ramping its sibling through 0/25/50/75/100%, to confirm
 *  changing one channel's duty doesn't disturb the other's.
 */

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("analogWrite all-channel + independence test");
}

void phaseA() {
  Serial.println("--- Phase A: distinct fixed duty on all 6 channels ---");

  struct { int pin; const char *name; uint8_t value; } chans[] = {
    { PWM0, "PWM0", 26 },   // ~10%
    { PWM1, "PWM1", 77 },   // ~30%
    { PWM2, "PWM2", 128 },  // ~50%
    { PWM3, "PWM3", 179 },  // ~70%
    { PWM4, "PWM4", 230 },  // ~90%
    { PWM5, "PWM5", 51 },   // ~20%
  };

  for (auto &c : chans) {
    Serial.print(c.name);
    Serial.print(" = ");
    Serial.println(c.value);
    analogWrite(c.pin, c.value);
  }

  Serial.println("holding for 8s ...");
  delay(8000);
}

void phaseB_pair(const char *fixedName, int fixedPin,
                  const char *sweepName, int sweepPin) {
  Serial.print("--- Phase B: ");
  Serial.print(fixedName);
  Serial.print(" fixed 50%, sweeping ");
  Serial.print(sweepName);
  Serial.println(" ---");

  analogWrite(fixedPin, 128);  // 50%, held for the whole phase

  uint8_t steps[] = { 0, 64, 128, 191, 255 };
  for (uint8_t v : steps) {
    Serial.print(sweepName);
    Serial.print(" = ");
    Serial.print(v);
    Serial.print(" (");
    Serial.print(fixedName);
    Serial.println(" should stay at 128/50%)");

    analogWrite(sweepPin, v);
    delay(3000);
  }
}

void loop() {
  phaseA();

  phaseB_pair("PWM1", PWM1, "PWM0", PWM0);
  phaseB_pair("PWM3", PWM3, "PWM2", PWM2);
  phaseB_pair("PWM5", PWM5, "PWM4", PWM4);

  Serial.println("--- cycle complete, repeating ---");
}
