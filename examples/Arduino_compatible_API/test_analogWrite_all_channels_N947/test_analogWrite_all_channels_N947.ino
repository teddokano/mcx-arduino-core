/** analogWrite() all-channel + independence test for FRDM-MCXN947
 *
 *  Drives all six PWM_0..PWM_5 pins (P2_2..P2_7, FlexPWM1) and checks two
 *  things a logic analyzer probed on all six lines at once can verify:
 *
 *  Phase A: all six channels set to distinct, fixed duty cycles
 *  simultaneously -- confirms every channel actually outputs (not just
 *  PWM_0) and that the six values are individually correct.
 *
 *  Phase B: PWM_0/PWM_1, PWM_2/PWM_3, and PWM_4/PWM_5 each share a FlexPWM
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
    { PWM_0, "PWM_0", 26 },   // ~10%
    { PWM_1, "PWM_1", 77 },   // ~30%
    { PWM_2, "PWM_2", 128 },  // ~50%
    { PWM_3, "PWM_3", 179 },  // ~70%
    { PWM_4, "PWM_4", 230 },  // ~90%
    { PWM_5, "PWM_5", 51 },   // ~20%
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

  phaseB_pair("PWM_1", PWM_1, "PWM_0", PWM_0);
  phaseB_pair("PWM_3", PWM_3, "PWM_2", PWM_2);
  phaseB_pair("PWM_5", PWM_5, "PWM_4", PWM_4);

  Serial.println("--- cycle complete, repeating ---");
}
