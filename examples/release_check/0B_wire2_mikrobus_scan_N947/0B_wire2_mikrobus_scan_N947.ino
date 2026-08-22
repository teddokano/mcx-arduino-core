/** Release check 0B: Wire2 (MikroBus I2C) bus-scan test, FRDM-MCXN947
 *  only -- Wire2 doesn't exist on A153 (that chip has only one I2C
 *  peripheral, already used by Wire), so this is numbered outside the
 *  01-06 no-external-hardware sequence rather than folded into it (see
 *  examples/release_check/README.md).
 *
 *  Mirrors examples/Arduino_compatible_API/test_Wire2_MikroBus_N947
 *  exactly.
 *
 *  Wire2 is a plain I2C instance on the MikroBus header's SDA/SCL pins
 *  (MB_SDA=P1_0, MB_SCL=P1_1), independent of Wire (D18/D19) and Wire1
 *  (I3C-in-I2C-mode, on-board sensor). Backed by its own peripheral
 *  (LPI2C3/FlexComm3), so all three can be used in the same sketch.
 *
 *  No specific device required -- this is just an i2c.scan() over
 *  addresses 0x08-0x77. Judgment: confirm on a logic analyzer that
 *  START/address/STOP traffic actually appears on the MikroBus SDA/SCL
 *  pins (with nothing connected, every address NAKs, so "0 device(s)
 *  found" every loop is expected and fine -- the point is that the bus
 *  itself is alive, not that a device answers).
 */

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Wire2 (MikroBus I2C) bus scan");

  Wire2.begin();
}

void loop() {
  int found = 0;

  for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
    Wire2.beginTransmission(addr);
    uint8_t err = Wire2.endTransmission();

    if (err == 0) {
      Serial.print("found device at 0x");
      Serial.println(addr, HEX);
      found++;
    }
  }

  Serial.print(found);
  Serial.println(" device(s) found");
  Serial.println("---");

  delay(2000);
}
