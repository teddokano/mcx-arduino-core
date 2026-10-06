/** mcxPinState: audit mcx-arduino-core's own combined-peripherals stress test
 *
 *  Mirrors the peripheral set exercised by mcx-arduino-core's own
 *  examples/Arduino_compatible_API/test_combined_peripherals.ino (I3C via
 *  Wire1, analogRead, analogWrite, tone, and on FRDM-MCXA153 also Serial1 +
 *  SPI1 on the MikroBus header; on FRDM-MCXA156, where the on-board sensor
 *  is on Wire and Wire1 is the MikroBus I2C, Wire + Wire1 + Serial1 +
 *  Serial2 + SPI1) but without needing that sketch's external
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
#if defined(FRDM_MCXA153) || defined(FRDM_MCXA156) || defined(FRDM_MCXN236)
#define ADC_PIN A0
#elif defined(FRDM_MCXN947)
#define ADC_PIN A2
#else
#error "This sketch has no settings for this board yet"
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
#elif defined(FRDM_MCXA156)
  // Every one of these has a peripheral of its own here: Wire (D18/D19,
  // with the on-board sensor), Wire1 (MikroBus I2C), Serial1 (D0/D1),
  // Serial2 (MikroBus UART) and SPI1 (MikroBus SPI)
  Wire.begin();
  Serial1.begin(9600);
  Serial2.begin(9600);

  pinMode(MB_CS, OUTPUT);
  digitalWrite(MB_CS, HIGH);
  SPI1.begin();
#elif defined(FRDM_MCXN236)
  // Wire (D18/D19) and Serial1 (D0/D1, sharing its FlexComm with Wire1).
  // No SPI: this board has no SPI1, and SPI's SCLK is D13, the tone pin
  Wire.begin();
  Serial1.begin(9600);
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
