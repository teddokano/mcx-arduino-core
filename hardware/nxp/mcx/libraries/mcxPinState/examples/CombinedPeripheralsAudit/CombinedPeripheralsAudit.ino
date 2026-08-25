/** mcxPinState: audit mcx-arduino-core's own combined-peripherals stress test
 *
 *  Mirrors the peripheral set exercised by mcx-arduino-core's own
 *  examples/Arduino_compatible_API/test_combined_peripherals.ino (I3C via
 *  Wire1, analogRead, analogWrite, tone, and on FRDM-MCXA153 also Serial1 +
 *  SPI1 on the MikroBus header) but without needing that sketch's external
 *  P3T1755 sensor library -- begin()ing Wire1 alone is enough to make its
 *  pins show up in the ownership table, without actually talking to the
 *  sensor.
 *
 *  Wiring: none required. This only checks pin ownership/MUX state, not
 *  actual peripheral function -- no loopback jumpers needed.
 *
 *  Expect: no "CONFLICT" or "MISMATCH" Status anywhere in either table.
 */

#include <Arduino.h>
#include <Wire.h>
#include <PinState.h>

PinState pins;

#define BUZZER_PIN D13
#define PWM_PIN    PWM0
#if defined(FRDM_MCXA153)
#define ADC_PIN A0
#elif defined(FRDM_MCXN947)
#define ADC_PIN A2
#endif

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Wire1.begin();

#if defined(FRDM_MCXA153)
  Serial1.begin(9600);

  pinMode(MB_CS, OUTPUT);
  digitalWrite(MB_CS, HIGH);
  SPI1.begin();
#endif

  analogRead(ADC_PIN);
  analogWrite(PWM_PIN, 128);
  tone(BUZZER_PIN, 880, 150);
  delay(200);  // let the brief tone burst finish before printing

  Serial.println();
  pins.print();
}

void loop() {
}
