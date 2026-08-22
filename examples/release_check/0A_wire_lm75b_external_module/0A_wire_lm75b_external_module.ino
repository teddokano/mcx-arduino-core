/** Release check 0A: Wire (plain I2C) real-device test -- needs an
 *  external LM75-family temperature sensor module, so it's numbered
 *  outside the 01-06 no-external-hardware sequence rather than folded
 *  into it (see examples/release_check/README.md).
 *
 *  Mirrors examples/Arduino_compatible_API/test_Wire_LM75B exactly.
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
 *
 *  Also exercises requestFrom()'s `stop` parameter, which used to be
 *  silently ignored (a fixed STOP was always issued regardless of what was
 *  passed). Each loop does two back-to-back reads: the first with
 *  stop=false (bus should be left held -- no STOP condition, straight into
 *  a repeated START for the next transaction), the second with stop=true
 *  (normal, closes the bus). On a logic analyzer capture, the boundary
 *  between the two reads should show a repeated START (Sr) with no STOP in
 *  between; if `stop` were still being ignored, a STOP would appear there
 *  before the second START instead.
 */

#include <Arduino.h>

const uint8_t SENSOR_ADDR = 0x72;
const uint8_t TEMP_REG = 0x00;

bool readTemp(bool stop, float &celsius) {
  Wire.beginTransmission(SENSOR_ADDR);
  Wire.write(TEMP_REG);
  uint8_t err = Wire.endTransmission(false);

  if (err != 0) {
    Serial.print("endTransmission failed, error = ");
    Serial.println(err);
    return false;
  }

  uint8_t n = Wire.requestFrom(SENSOR_ADDR, (size_t)2, stop);

  if (n != 2) {
    Serial.print("requestFrom returned ");
    Serial.print(n);
    Serial.println(" bytes, expected 2");
    return false;
  }

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();

  int16_t raw = (int16_t)((msb << 8) | lsb);
  raw >>= 5;
  celsius = raw * 0.125f;

  return true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Wire (plain I2C) + LM75-family sensor test (with requestFrom(stop) check)");

  Wire.begin();
}

void loop() {
  float t1, t2;

  Serial.println("--- requestFrom(..., stop=false) ---");
  bool ok1 = readTemp(false, t1);

  Serial.println("--- requestFrom(..., stop=true) ---");
  bool ok2 = readTemp(true, t2);

  if (ok1)
    Serial.println("stop=false read: OK, temp = " + String(t1, 3) + " degC");
  if (ok2)
    Serial.println("stop=true  read: OK, temp = " + String(t2, 3) + " degC");

  delay(1000);
}
