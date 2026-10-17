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

  Serial.println("analogRead / analogWrite test");
}

void loop() {
  int value = analogRead(A2);

  analogWrite(PWM_PIN, value >> 2);   // 10bit -> 8bit

  Serial.print("A2 = ");
  Serial.println(value);

  delay(200);
}
