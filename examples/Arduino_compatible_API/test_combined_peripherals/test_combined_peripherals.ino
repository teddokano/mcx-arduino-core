/** Combined peripheral stress test
 *
 *  Exercises all newly-added peripherals together in a single loop to check
 *  for interference between them:
 *    - I3C/Wire1  : on-board P3T1755 temperature sensor (existing feature)
 *    - LPADC      : analogRead(A0)
 *    - FlexPWM0   : analogWrite(PWM0) with a changing duty cycle every loop
 *    - CTIMER0    : tone()/noTone() burst, interrupting on top of everything else
 *    - SysTick/DWT: millis()/micros(), printed every loop (runs continuously
 *                   throughout regardless of what else is happening)
 *
 *  If any of these peripherals share a clock/interrupt resource incorrectly,
 *  expect symptoms here: I2C/I3C read errors or hangs, out-of-range ADC
 *  values, PWM/tone glitches, or millis()/micros() drifting/stalling.
 */

#include <Arduino.h>
#include <P3T1755.h>
#include <Wire.h>

#define BUZZER_PIN  D13
#define PWM_PIN     PWM0
#define ADC_PIN     A0

P3T1755 sensor(Wire1, 0x48);

int pwmDuty = 0;
int pwmStep = 5;
int loopCount = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Wire1.begin();

  Serial.println("Combined peripheral test: I3C(Wire1) + analogRead + analogWrite + tone + millis/micros");
}

void loop() {
  unsigned long ms = millis();
  unsigned long us = micros();

  // I3C (on-board P3T1755 over Wire1)
  float temp = sensor.temp();

  // LPADC
  int adc = analogRead(ADC_PIN);

  // FlexPWM0 -- breathing duty, reconfigured every loop
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

  // CTIMER0 -- short tone burst every 4th loop, overlapping I2C/ADC/PWM activity
  if ((loopCount++ % 4) == 0)
    tone(BUZZER_PIN, 880, 150);

  delay(200);
}
