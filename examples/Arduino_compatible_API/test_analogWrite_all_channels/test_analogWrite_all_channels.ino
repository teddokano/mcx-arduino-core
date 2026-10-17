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
 *
 *  FRDM-MCXC444 has no PWM0-PWM5 pins. Its PWM is on three TPM channels:
 *  D3 (TPM1_CH0), D10 (TPM0_CH4), and D5/D6/D9, which all share TPM0_CH2.
 *  Phase A drives D3, D9 and D10; phase B's pair is D10 and D9, the two
 *  channels of TPM0, which share its period.
 */

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("analogWrite all-channel + independence test");
}

void phaseA() {
  Serial.println("--- Phase A: distinct fixed duty on every channel ---");

  struct { int pin; const char *name; uint8_t value; } chans[] = {
#if defined(FRDM_MCXC444)
    { D3, "D3", 26 },     // ~10%
    { D9, "D9", 128 },    // ~50%
    { D10, "D10", 230 },  // ~90%
#else
    { PWM0, "PWM0", 26 },   // ~10%
    { PWM1, "PWM1", 77 },   // ~30%
    { PWM2, "PWM2", 128 },  // ~50%
    { PWM3, "PWM3", 179 },  // ~70%
    { PWM4, "PWM4", 230 },  // ~90%
    { PWM5, "PWM5", 51 },   // ~20%
#endif
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

#if defined(FRDM_MCXC444)
  phaseB_pair("D10", D10, "D9", D9);
#else
  phaseB_pair("PWM1", PWM1, "PWM0", PWM0);
  phaseB_pair("PWM3", PWM3, "PWM2", PWM2);
  phaseB_pair("PWM5", PWM5, "PWM4", PWM4);
#endif

  Serial.println("--- cycle complete, repeating ---");
}
