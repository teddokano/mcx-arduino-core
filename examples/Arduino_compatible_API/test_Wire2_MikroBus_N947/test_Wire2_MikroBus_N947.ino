/** Wire2 (MikroBus I2C) bus-scan test for FRDM-MCXN947
 *
 *  Wire2 is a plain I2C instance on the MikroBus header's SDA/SCL pins
 *  (MB_SDA=P1_0, MB_SCL=P1_1), independent of Wire (D18/D19) and Wire1
 *  (I3C-in-I2C-mode, on-board sensor). Backed by its own peripheral
 *  (LPI2C3/FlexComm3), so all three can be used in the same sketch.
 *
 *  Scans addresses 0x08-0x77 and reports what ACKs. No specific device
 *  required -- plug any I2C MikroBus Click board's SDA/SCL in to see it
 *  show up, or run with nothing connected to confirm the bus itself
 *  doesn't hang/crash.
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
