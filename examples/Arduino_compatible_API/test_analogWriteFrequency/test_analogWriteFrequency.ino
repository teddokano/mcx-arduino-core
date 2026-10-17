/*
 * Verifies analogWriteFrequency(pin, hz) -- a non-standard extension
 * (not part of the official Arduino API; modeled on Teensy's function of
 * the same name) that sets a PWM pin's period, independent of
 * analogWriteResolution()'s bit depth.
 *
 * Cycles PWM0 through a few frequencies, re-asserting 50% duty at each
 * one via analogWrite() right after -- the natural call order for real
 * use (analogWriteFrequency() to pick the rate, analogWrite() to set the
 * duty at that rate). Check the period/duty on a scope or logic
 * analyzer against the values printed to Serial.
 *
 * Portable across boards: PWM0 is common to FRDM-MCXA153/FRDM-MCXN947.
 * FRDM-MCXC444 has no PWM0-PWM5 pins, and uses D3 instead.
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

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  pinMode(PWM_PIN, OUTPUT);

  Serial.println("analogWriteFrequency test -- " PWM_PIN_NAME ", 50% duty at each frequency");
}

void loop() {
  static const uint32_t freqs[] = { 1000, 50, 20, 5000 };
  static size_t i = 0;

  uint32_t f = freqs[i];

  analogWriteFrequency(PWM_PIN, f);
  analogWrite(PWM_PIN, 128);  // 50% duty (8bit default resolution) at the new frequency

  Serial.printf(PWM_PIN_NAME ": %lu Hz (period %lu us), 50%% duty\r\n",
                (unsigned long)f, (unsigned long)(1000000UL / f));

  i = (i + 1) % (sizeof(freqs) / sizeof(freqs[0]));
  delay(4000);
}
