/** Combined peripheral stress test -- FRDM-MCXN947
 *
 *  Exercises multiple peripherals together in a single loop to check for
 *  interference between them:
 *    - I3C/Wire1  : on-board P3T1755 temperature sensor
 *    - LPADC      : analogRead(A2)
 *    - FlexPWM1   : analogWrite(PWM_0) with a changing duty cycle every loop
 *    - CTIMER0    : tone()/noTone() burst, interrupting on top of everything else
 *    - SysTick/DWT: millis()/micros(), printed every loop (runs continuously
 *                   throughout regardless of what else is happening)
 *
 *  Unlike the A153 version of this test, Serial1 is NOT included here: on
 *  this board Serial1 lives on the MikroBus header (MB_TX/MB_RX), which are
 *  the exact same physical pins Wire1 uses for I3C. The two are mutually
 *  exclusive on those pins (see variants/frdm_mcxn947/README.md) -- running
 *  both in the same loop would fight over the same PORT mux, not stress-test
 *  genuinely independent peripherals. Wire2 (MB_SDA/MB_SCL, LPI2C3) and SPI1
 *  (MikroBus SPI, LPSPI6) don't have this conflict and could be added here
 *  if a wider stress test is ever needed.
 *
 *  If any of these peripherals share a clock/interrupt resource incorrectly,
 *  expect symptoms here: I3C read errors or hangs, out-of-range ADC values,
 *  PWM/tone glitches, or millis()/micros() drifting/stalling.
 */

#include <Arduino.h>
#include <P3T1755.h>
#include <Wire.h>

#define BUZZER_PIN  D13
#define PWM_PIN     PWM_0
#define ADC_PIN     A2

P3T1755 sensor(Wire1, 0x48);

int pwmDuty = 0;
int pwmStep = 5;
int loopCount = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Wire1.begin();

  Serial.println("Combined peripheral test (N947): I3C(Wire1) + analogRead + analogWrite + tone + millis/micros");
}

void loop() {
  unsigned long ms = millis();
  unsigned long us = micros();

  // I3C (on-board P3T1755 over Wire1)
  float temp = sensor.temp();

  // LPADC
  int adc = analogRead(ADC_PIN);

  // FlexPWM1 -- breathing duty, reconfigured every loop
  pwmDuty += pwmStep;
  if (pwmDuty <= 0 || pwmDuty >= 255)
    pwmStep = -pwmStep;
  analogWrite(PWM_PIN, pwmDuty);

  Serial.print("millis=");
  Serial.print(ms);
  Serial.print(" micros=");
  Serial.print(us);
  Serial.print(" temp=");
  Serial.print(temp, 2);
  Serial.print(" adc=");
  Serial.print(adc);
  Serial.print(" pwmDuty=");
  Serial.print(pwmDuty);

  if (adc < 0 || adc > 1023)
    Serial.print("  <-- WARNING: analogRead out of range!");

  if (temp < -40.0 || temp > 125.0)
    Serial.print("  <-- WARNING: temp out of range!");

  Serial.println();

  // CTIMER0 -- short tone burst every 4th loop, overlapping I3C/ADC/PWM activity
  if ((loopCount % 4) == 0)
    tone(BUZZER_PIN, 880, 150);

  loopCount++;
  delay(200);
}
