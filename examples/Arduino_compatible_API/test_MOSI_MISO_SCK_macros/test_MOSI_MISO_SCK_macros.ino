/** Standard MOSI/MISO/SCK macro test
 *
 *  Confirms the bare MOSI/MISO/SCK identifiers (as provided by every other
 *  Arduino core, and required by third-party libraries like the official
 *  SD library) exist and are this board's default SPI pins, D11/D12/D13
 *  with SS on D10 (the ones the global `SPI` instance uses), compared by
 *  the pin they stand for (the names number pins differently).
 *  No wiring needed -- this only checks that the values compile and agree.
 */

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("MOSI/MISO/SCK macro test");

  Serial.print("MOSI is D11: ");
  Serial.println(arduino_pin_by_number[MOSI] == arduino_pin_by_number[D11] ? "OK" : "FAIL");

  Serial.print("MISO is D12: ");
  Serial.println(arduino_pin_by_number[MISO] == arduino_pin_by_number[D12] ? "OK" : "FAIL");

  Serial.print("SCK is D13: ");
  Serial.println(arduino_pin_by_number[SCK] == arduino_pin_by_number[D13] ? "OK" : "FAIL");

  Serial.print("SS is D10: ");
  Serial.println(arduino_pin_by_number[SS] == arduino_pin_by_number[D10] ? "OK" : "FAIL");
}

void loop() {
}
