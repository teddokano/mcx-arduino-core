/** SPI transfer16() / bitOrder / end() test
 *
 *  Needs a MOSI-MISO loopback wire (same setup as
 *  test_SPI_loopback_with_a_wire). Loopback can't distinguish wire-level
 *  bit order (MISO always echoes exactly what MOSI sent, byte-for-byte),
 *  but it does confirm: transfer16() round-trips correctly, switching
 *  bitOrder doesn't break subsequent transfers, and end()/begin() can
 *  tear down and restart cleanly.
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

  Serial.println("SPI transfer16 / bitOrder / end test");

  pinMode(SS, OUTPUT);
  digitalWrite(SS, HIGH);
  SPI.begin();

  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(SS, LOW);
  uint8_t b = SPI.transfer(0xA5);
  digitalWrite(SS, HIGH);
  check("transfer(uint8_t) loopback", b == 0xA5);

  digitalWrite(SS, LOW);
  uint16_t w1 = SPI.transfer16(0x1234);
  digitalWrite(SS, HIGH);
  check("transfer16 (MSBFIRST) loopback", w1 == 0x1234);
  SPI.endTransaction();

  // switch bit order mid-session -- should not break subsequent transfers
  SPI.beginTransaction(SPISettings(1000000, LSBFIRST, SPI_MODE0));
  digitalWrite(SS, LOW);
  uint16_t w2 = SPI.transfer16(0x5678);
  digitalWrite(SS, HIGH);
  check("transfer16 (LSBFIRST) loopback", w2 == 0x5678);
  SPI.endTransaction();

  // end() then begin() again -- should be able to restart cleanly
  SPI.end();
  SPI.begin();
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(SS, LOW);
  uint8_t b2 = SPI.transfer(0x5A);
  digitalWrite(SS, HIGH);
  SPI.endTransaction();
  check("transfer after end()+begin()", b2 == 0x5A);
}

void loop() {
}
