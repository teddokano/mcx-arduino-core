/** Combined peripheral stress test -- FRDM-MCXA153 / FRDM-MCXN947
 *
 *  Exercises multiple peripherals together in a single loop to check for
 *  interference between them:
 *    - I3C/Wire1  : on-board P3T1755 temperature sensor
 *    - LPADC      : analogRead()
 *    - FlexPWM    : analogWrite(PWM0) with a changing duty cycle every loop
 *    - CTIMER0    : tone()/noTone() burst, interrupting on top of everything else
 *    - SysTick/DWT: millis()/micros(), printed every loop (runs continuously
 *                   throughout regardless of what else is happening)
 *    - SPI1       : MikroBus SPI loopback (requires a MB_MOSI->MB_MISO
 *                   jumper wire) -- own peripheral, independent of
 *                   everything else here, exercised on both boards
 *    - Serial1    : (A153 only) D0/D1 hardware UART loopback (requires a
 *                   D1->D0 jumper wire)
 *    - Wire2      : (N947 only) MikroBus I2C (MB_SDA/MB_SCL) -- own
 *                   peripheral (LPI2C3/FlexComm3), independent of
 *                   everything else here. No device required: this just
 *                   probes a fixed address every loop and expects a NAK
 *                   (err != 0) back promptly, the same way the bus-scan
 *                   in examples/release_check/0B works with nothing
 *                   plugged in -- the point here is that it completes
 *                   promptly under load, not that anything ACKs
 *
 *  Serial1 is only exercised on FRDM-MCXA153. On FRDM-MCXN947, Serial1
 *  lives on the MikroBus header (MB_TX/MB_RX), which are the exact same
 *  physical pins Wire1 uses for I3C -- the two are mutually exclusive on
 *  those pins (see variants/frdm_mcxn947/README.md), so running both in
 *  the same loop would fight over the same PORT mux, not stress-test
 *  genuinely independent peripherals. Wire2 doesn't have this conflict,
 *  so it takes Serial1's place in the N947 branch instead.
 *
 *  If any of these peripherals share a clock/interrupt resource incorrectly,
 *  expect symptoms here: I2C/I3C read errors or hangs, out-of-range ADC
 *  values, PWM/tone glitches, millis()/micros() drifting/stalling, Serial1
 *  bytes going missing, SPI1 transfers not echoing correctly, or Wire2
 *  probes taking noticeably longer than the others to return.
 */

#include <Arduino.h>
#include <P3T1755.h>
#include <Wire.h>

#define BUZZER_PIN  D13
#define PWM_PIN     PWM0
#if defined(FRDM_MCXA153)
#define ADC_PIN     A0
#elif defined(FRDM_MCXN947)
#define ADC_PIN     A2
#endif

P3T1755 sensor(Wire1, 0x48);

int pwmDuty = 0;
int pwmStep = 5;
int loopCount = 0;
#if defined(FRDM_MCXA153)
char serial1Rx[32];
#endif

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Wire1.begin();
#if defined(FRDM_MCXA153)
  Serial1.begin(9600);  // D0/D1 hardware UART -- jumper D1->D0 to loop back
#elif defined(FRDM_MCXN947)
  Wire2.begin();  // MikroBus I2C -- no device required, see header comment
#endif

  pinMode(MB_CS, OUTPUT);
  digitalWrite(MB_CS, HIGH);
  SPI1.begin();  // MikroBus SPI -- jumper MB_MOSI->MB_MISO to loop back
  SPI1.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

#if defined(FRDM_MCXA153)
  Serial.println("Combined peripheral test: I3C(Wire1) + analogRead + analogWrite + tone + millis/micros + Serial1 + SPI1");
#elif defined(FRDM_MCXN947)
  Serial.println("Combined peripheral test (N947): I3C(Wire1) + analogRead + analogWrite + tone + millis/micros + Wire2 + SPI1");
#endif
}

void loop() {
  unsigned long ms = millis();
  unsigned long us = micros();

#if defined(FRDM_MCXA153)
  // Serial1 -- read back whatever was sent at the end of the previous loop
  // iteration; the 200ms loop period gives it time to arrive over the
  // D1->D0 jumper.
  int serial1Avail = Serial1.available();
  int i = 0;
  while (Serial1.available() && i < (int)sizeof(serial1Rx) - 1)
    serial1Rx[i++] = (char)Serial1.read();
  serial1Rx[i] = '\0';
#endif

  // I3C (on-board P3T1755 over Wire1)
  float temp = sensor.temp();

  // LPADC
  int adc = analogRead(ADC_PIN);

  // FlexPWM -- breathing duty, reconfigured every loop
  pwmDuty += pwmStep;
  if (pwmDuty <= 0 || pwmDuty >= 255)
    pwmStep = -pwmStep;
  analogWrite(PWM_PIN, pwmDuty);

  // SPI1 (MikroBus) -- loopback via MB_MOSI->MB_MISO jumper
  digitalWrite(MB_CS, LOW);
  uint16_t spi1Echo = SPI1.transfer16(0x1234);
  digitalWrite(MB_CS, HIGH);

#if defined(FRDM_MCXN947)
  // Wire2 (MikroBus I2C) -- no device expected, just probing that the bus
  // completes promptly under load; see header comment
  Wire2.beginTransmission(0x08);
  uint8_t wire2Err = Wire2.endTransmission();
#endif

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
  Serial.print(" spi1Echo=0x");
  Serial.print(spi1Echo, HEX);
#if defined(FRDM_MCXA153)
  Serial.print(" serial1=\"");
  Serial.print(serial1Rx);
  Serial.print("\"");
#elif defined(FRDM_MCXN947)
  Serial.print(" wire2Err=");
  Serial.print(wire2Err);
#endif

  if (adc < 0 || adc > 1023)
    Serial.print("  <-- WARNING: analogRead out of range!");

  if (temp < -40.0 || temp > 125.0)
    Serial.print("  <-- WARNING: temp out of range!");

  if (spi1Echo != 0x1234)
    Serial.print("  <-- WARNING: SPI1 loopback echo mismatch!");

#if defined(FRDM_MCXA153)
  if (loopCount > 0 && serial1Avail == 0)
    Serial.print("  <-- WARNING: Serial1 loopback got nothing!");
#endif

  Serial.println();

  // CTIMER0 -- short tone burst every 4th loop, overlapping I2C/ADC/PWM activity
  if ((loopCount % 4) == 0)
    tone(BUZZER_PIN, 880, 150);

#if defined(FRDM_MCXA153)
  // Serial1 -- send this loop's marker; read back at the top of the next
  // iteration
  Serial1.print("hb");
  Serial1.println(loopCount);
#endif

  loopCount++;
  delay(200);
}
