/** I2C controller (master) demo -- pair it with Wire_target_demo on
 *  another board.
 *
 *  Once a second this board:
 *    - writes a line of text to the target at address 0x08
 *    - reads 7 bytes back from it
 *  and prints what happened. If the target isn't there (not wired, or
 *  not running yet), it says so and keeps trying.
 *
 *  Wiring, between the two boards:
 *    D18 (SDA) -- D18 (SDA)
 *    D19 (SCL) -- D19 (SCL)
 *    GND       -- GND
 *
 *  No pull-up resistors needed for these short wires: Wire.begin() turns
 *  on the pins' internal ones. A longer bus, or a faster one, wants
 *  external pull-ups as well (e.g. 4.7k to 3.3V on SDA and SCL).
 *
 *  Works on FRDM-MCXA153 and FRDM-MCXN947.
 */

#include <Arduino.h>
#include <Wire.h>

const uint8_t TARGET_ADDRESS = 0x08;

int count = 0;

void setup() {
  Serial.begin(115200);

  // A timeout keeps a transfer from hanging if the other board is reset
  // in the middle of it
  Wire.setWireTimeout(25000);
  Wire.begin();

  Serial.println("I2C controller ready");
}

void loop() {
  char text[24];
  snprintf(text, sizeof(text), "hello #%d", count);

  // Write
  Wire.beginTransmission(TARGET_ADDRESS);
  Wire.print(text);
  uint8_t result = Wire.endTransmission();

  Serial.print("sent \"");
  Serial.print(text);
  Serial.print("\" -> ");
  if (result == 0) {
    Serial.print("OK");
  } else {
    Serial.print("no answer (");
    Serial.print(result);
    Serial.print(")");
  }

  // Read
  if (result == 0) {
    uint8_t n = Wire.requestFrom(TARGET_ADDRESS, (size_t)7);
    Serial.print(", reply: \"");
    while (Wire.available())
      Serial.print((char)Wire.read());
    Serial.print("\" (");
    Serial.print(n);
    Serial.print(" bytes)");
  }
  Serial.println();

  count++;
  delay(1000);
}
