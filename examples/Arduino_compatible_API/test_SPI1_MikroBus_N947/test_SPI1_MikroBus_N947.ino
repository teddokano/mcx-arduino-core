/** SPI1 (MikroBus SPI) loopback test for FRDM-MCXN947
 *
 *  SPI1 is a plain SPI instance on the MikroBus header's MOSI/MISO/SCK/CS
 *  pins (P3_20/22/21/23), independent of SPI (D10-D13). Backed by its own
 *  peripheral (LPSPI6/FlexComm6), so both can be used in the same sketch.
 *
 *  Needs a MOSI-MISO loopback wire on the MikroBus header. Runs all
 *  transfers back-to-back before printing results, so a logic analyzer
 *  capture isn't interrupted by Serial calls.
 */

#include <Arduino.h>

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("SPI1 (MikroBus SPI) loopback test");

  pinMode(MB_CS, OUTPUT);
  digitalWrite(MB_CS, HIGH);
  SPI1.begin();

  SPI1.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

  digitalWrite(MB_CS, LOW);
  uint8_t b1 = SPI1.transfer(0xA5);
  digitalWrite(MB_CS, HIGH);

  digitalWrite(MB_CS, LOW);
  uint16_t w1 = SPI1.transfer16(0x1234);
  digitalWrite(MB_CS, HIGH);

  SPI1.endTransaction();

  check("transfer(uint8_t) loopback", b1 == 0xA5);
  check("transfer16 loopback", w1 == 0x1234);
}

void loop() {
}
