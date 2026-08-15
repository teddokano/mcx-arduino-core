/** Standard MOSI/MISO/SCK macro test
 *
 *  Confirms the bare MOSI/MISO/SCK identifiers (as provided by every other
 *  Arduino core, and required by third-party libraries like the official
 *  SD library) exist and match this board's default SPI pins
 *  (ARD_MOSI/ARD_MISO/ARD_SCK, the ones the global `SPI` instance uses).
 *  No wiring needed -- this only checks that the values compile and agree.
 */

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("MOSI/MISO/SCK macro test");

  Serial.print("MOSI == ARD_MOSI: ");
  Serial.println(MOSI == ARD_MOSI ? "OK" : "FAIL");

  Serial.print("MISO == ARD_MISO: ");
  Serial.println(MISO == ARD_MISO ? "OK" : "FAIL");

  Serial.print("SCK == ARD_SCK: ");
  Serial.println(SCK == ARD_SCK ? "OK" : "FAIL");
}

void loop() {
}
