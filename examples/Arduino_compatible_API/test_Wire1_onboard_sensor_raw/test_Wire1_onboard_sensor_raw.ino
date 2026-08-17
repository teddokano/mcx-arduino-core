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
 */

#include <Arduino.h>

const uint8_t SENSOR_ADDR = 0x48;
const uint8_t TEMP_REG = 0x00;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Wire1 (on-board I3C-in-I2C-mode) + P3T1755 raw register test");

  Wire1.begin();
}

void loop() {
  Wire1.beginTransmission(SENSOR_ADDR);
  Wire1.write(TEMP_REG);
  uint8_t err = Wire1.endTransmission(false);  // repeated start, keep the bus held

  if (err != 0) {
    Serial.print("endTransmission failed, error = ");
    Serial.println(err);
    delay(1000);
    return;
  }

  uint8_t n = Wire1.requestFrom(SENSOR_ADDR, (size_t)2);

  if (n != 2) {
    Serial.print("requestFrom returned ");
    Serial.print(n);
    Serial.println(" bytes, expected 2");
    delay(1000);
    return;
  }

  uint8_t msb = Wire1.read();
  uint8_t lsb = Wire1.read();

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
