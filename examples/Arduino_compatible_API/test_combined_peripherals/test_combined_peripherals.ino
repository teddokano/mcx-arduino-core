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
 *    - Serial1    : D0/D1 hardware UART loopback (requires a D1->D0 jumper wire)
 *
 *  If any of these peripherals share a clock/interrupt resource incorrectly,
 *  expect symptoms here: I2C/I3C read errors or hangs, out-of-range ADC
 *  values, PWM/tone glitches, millis()/micros() drifting/stalling, or
 *  Serial1 bytes going missing.
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
char serial1Rx[32];

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Wire1.begin();
  Serial1.begin(9600);  // D0/D1 hardware UART -- jumper D1->D0 to loop back

  Serial.println("Combined peripheral test: I3C(Wire1) + analogRead + analogWrite + tone + millis/micros + Serial1");
}

void loop() {
  unsigned long ms = millis();
  unsigned long us = micros();

  // Serial1 -- read back whatever was sent at the end of the previous loop
  // iteration; the 200ms loop period gives it time to arrive over the
  // D1->D0 jumper.
  int serial1Avail = Serial1.available();
  int i = 0;
  while (Serial1.available() && i < (int)sizeof(serial1Rx) - 1)
    serial1Rx[i++] = (char)Serial1.read();
  serial1Rx[i] = '\0';

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
  Serial.print(" serial1=\"");
  Serial.print(serial1Rx);
  Serial.print("\"");

  if (adc < 0 || adc > 1023)
    Serial.print("  <-- WARNING: analogRead out of range!");

  if (temp < -40.0 || temp > 125.0)
    Serial.print("  <-- WARNING: temp out of range!");

  if (loopCount > 0 && serial1Avail == 0)
    Serial.print("  <-- WARNING: Serial1 loopback got nothing!");

  Serial.println();

  // CTIMER0 -- short tone burst every 4th loop, overlapping I2C/ADC/PWM activity
  if ((loopCount % 4) == 0)
    tone(BUZZER_PIN, 880, 150);

  // Serial1 -- send this loop's marker; read back at the top of the next
  // iteration
  Serial1.print("hb");
  Serial1.println(loopCount);

  loopCount++;
  delay(200);
}
