/** Wire1 (on-board I3C-in-I2C-mode) temperature read, no external
 *  library required -- talks to the on-board P3T1755's register
 *  interface directly with beginTransmission()/write()/endTransmission()/
 *  requestFrom()/read(), the same way you'd talk to any I2C device
 *  without a driver library. No wiring needed -- the sensor is on-board.
 *
 *  P3T1755's temperature register (pointer 0x00): 2 bytes, MSB first,
 *  11-bit two's complement value left-justified in the 16-bit word (bits
 *  15:5), 0.125 degC per LSB at bit 5 -- so shifting the raw 16-bit word
 *  right by 5 (arithmetic shift, sign-extending) and scaling by 0.125
 *  gives degC directly. Same register format as the LM75B/P3T1035x family
 *  -- see test_Wire_LM75B for the same technique on the external `Wire`
 *  instance instead.
 *
 *  On FRDM-MCXA156 the sensor is on Wire (D18/D19) instead, and the sketch
 *  uses that; the messages still say Wire1.
 */

#include <Arduino.h>

// The on-board P3T1755's bus: Wire1 on FRDM-MCXA153 and FRDM-MCXN947,
// Wire (D18/D19) on FRDM-MCXA156, whose sensor is on the Arduino I2C pins
// FRDM-MCXN236 has none on board: connect one to its MikroBus I2C (Wire1)
#if defined(FRDM_MCXA153) || defined(FRDM_MCXN947) || defined(FRDM_MCXN236)
#define SENSOR_WIRE Wire1
#elif defined(FRDM_MCXA156)
#define SENSOR_WIRE Wire
#else
#error "This sketch has no settings for this board yet"
#endif

const uint8_t SENSOR_ADDR = 0x48;
const uint8_t TEMP_REG = 0x00;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Wire1 (on-board I3C-in-I2C-mode) + P3T1755 raw register test");

  SENSOR_WIRE.begin();
}

void loop() {
  SENSOR_WIRE.beginTransmission(SENSOR_ADDR);
  SENSOR_WIRE.write(TEMP_REG);
  uint8_t err = SENSOR_WIRE.endTransmission(false);  // repeated start, keep the bus held

  if (err != 0) {
    Serial.print("endTransmission failed, error = ");
    Serial.println(err);
    delay(1000);
    return;
  }

  uint8_t n = SENSOR_WIRE.requestFrom(SENSOR_ADDR, (size_t)2);

  if (n != 2) {
    Serial.print("requestFrom returned ");
    Serial.print(n);
    Serial.println(" bytes, expected 2");
    delay(1000);
    return;
  }

  uint8_t msb = SENSOR_WIRE.read();
  uint8_t lsb = SENSOR_WIRE.read();

  int16_t raw = (int16_t)((msb << 8) | lsb);
  raw >>= 5;
  float celsius = raw * 0.125f;

  Serial.print("raw = 0x");
  Serial.print(msb, HEX);
  Serial.print(lsb, HEX);
  Serial.print("  temp = ");
  Serial.print(celsius, 3);
  Serial.println(" degC");

  delay(1000);
}
