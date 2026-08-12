/** SPI legacy (pre-1.6) API test: setBitOrder/setDataMode/setClockDivider
 *
 *  Needs a MOSI-MISO loopback wire (same setup as
 *  test_SPI_loopback_with_a_wire). Loopback confirms transfers still work
 *  correctly after each legacy call reconfigures the hardware outside of
 *  a beginTransaction()/endTransaction() pair.
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

  Serial.println("SPI legacy API test");

  pinMode(SS, OUTPUT);
  digitalWrite(SS, HIGH);
  SPI.begin();

  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV8);

  digitalWrite(SS, LOW);
  uint8_t b1 = SPI.transfer(0x3C);
  digitalWrite(SS, HIGH);
  check("transfer after legacy setup (DIV8)", b1 == 0x3C);

  SPI.setBitOrder(LSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV32);

  digitalWrite(SS, LOW);
  uint8_t b2 = SPI.transfer(0xC3);
  digitalWrite(SS, HIGH);
  check("transfer after changing bitOrder/divider (DIV32)", b2 == 0xC3);

  SPI.setDataMode(SPI_MODE2);

  digitalWrite(SS, LOW);
  uint8_t b3 = SPI.transfer(0x5A);
  digitalWrite(SS, HIGH);
  check("transfer after changing dataMode", b3 == 0x5A);
}

void loop() {
}
