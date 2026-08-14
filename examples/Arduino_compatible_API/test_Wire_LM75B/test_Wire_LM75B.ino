/** Wire (plain I2C) real-device test using an external LM75-family
 *  temperature sensor
 *
 *  This is the first real-device (not just bus-scan) test of `Wire` -- the
 *  Arduino-connector I2C pins (D18/D19), as opposed to `Wire1` which is the
 *  on-board I3C-in-I2C-mode sensor. Verified on real hardware with a
 *  P3T1035xUK-ARD board wired to D18(SDA)/D19(SCL)/3V3/GND (7-bit address
 *  0x72 as tied on that board -- change SENSOR_ADDR below for an LM75B/
 *  P3T1755 at their more common default of 0x48).
 *
 *  LM75-family temperature register (pointer 0x00): 2 bytes, MSB first,
 *  11-bit two's complement value left-justified in the 16-bit word (bits
 *  15:5), 0.125 degC per LSB at bit 5 -- so shifting the raw 16-bit word
 *  right by 5 (arithmetic shift, sign-extending) and scaling by 0.125
 *  gives degC directly.
 */

#include <Arduino.h>

const uint8_t SENSOR_ADDR = 0x72;
const uint8_t TEMP_REG = 0x00;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Wire (plain I2C) + LM75-family sensor test");

  Wire.begin();
}

void loop() {
  Wire.beginTransmission(SENSOR_ADDR);
  Wire.write(TEMP_REG);
  uint8_t err = Wire.endTransmission(false);

  if (err != 0) {
    Serial.print("endTransmission failed, error = ");
    Serial.println(err);
    delay(1000);
    return;
  }

  uint8_t n = Wire.requestFrom(SENSOR_ADDR, (size_t)2);

  if (n != 2) {
    Serial.print("requestFrom returned ");
    Serial.print(n);
    Serial.println(" bytes, expected 2");
    delay(1000);
    return;
  }

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();

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
