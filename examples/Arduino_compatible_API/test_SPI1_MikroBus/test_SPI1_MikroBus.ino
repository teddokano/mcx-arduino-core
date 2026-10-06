/** SPI1 (MikroBus SPI) loopback test -- FRDM-MCXA153 / FRDM-MCXN947
 *
 *  SPI1 is a plain SPI instance on the MikroBus header's MOSI/MISO/SCK/CS
 *  pins, independent of SPI (D10-D13) -- backed by its own peripheral on
 *  each board (A153: LPSPI0, vs. SPI's LPSPI1; N947: LPSPI6/FlexComm6), so
 *  both SPI and SPI1 can be used in the same sketch. See each board's
 *  PIN_MAPPING_*.md for the exact physical pins.
 *
 *  Needs a MOSI-MISO loopback wire on the MikroBus header. Runs all
 *  transfers back-to-back before printing results, so a logic analyzer
 *  capture isn't interrupted by Serial calls.
 *
 *  This sketch needs no per-board #if branching -- SPI1/MB_CS already
 *  resolve to the right pins/peripheral on either board.
 *
 *  FRDM-MCXN236 has no SPI1: its MikroBus SPI is SPI's own D11-D13 lines,
 *  with CS on D18. There the sketch only says so.
 */

#include <Arduino.h>

#if defined(FRDM_MCXN236)
void setup() {
  Serial.begin(115200);
  Serial.println("FRDM-MCXN236 has no SPI1 (its MikroBus SPI is SPI's own lines)");
}

void loop() {
}
#elif defined(FRDM_MCXA153) || defined(FRDM_MCXA156) || defined(FRDM_MCXN947)

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
#else
#error "This sketch has no settings for this board yet"
#endif
