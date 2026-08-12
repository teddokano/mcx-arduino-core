/** Serial.print(x, BIN) and Serial.write() overloads test
 *
 *  BIN checks are pure logic (known values -> known binary strings) over
 *  the USB Serial. The write() overload checks need Serial1 (D1->D0
 *  jumper) as a loopback source, to confirm bytes actually go out and
 *  come back correctly -- including a null byte, to prove the buffer
 *  overload is count-based, not string-based.
 */

#include <Arduino.h>
#include <cstring>

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial1.begin(9600);

  Serial.println("Serial BIN / write() test");

  // ---- BIN base ----
  Serial.print("255 in BIN: ");
  Serial.println(255, BIN);
  Serial.print("5 in BIN: ");
  Serial.println(5, BIN);
  Serial.print("0 in BIN: ");
  Serial.println(0, BIN);
  Serial.print("-5 in BIN: ");
  Serial.println(-5, BIN);

  // capture via String to check programmatically (String's own toInt/etc.
  // aren't base-aware, so just re-derive the expected strings by hand)
  check("255 BIN == 11111111", String(255, BIN) == "11111111");
  check("5 BIN == 101", String(5, BIN) == "101");
  check("0 BIN == 0", String(0, BIN) == "0");
  check("-5 BIN == -101", String(-5, BIN) == "-101");

  // ---- write() overloads (Serial1 loopback) ----
  size_t n1 = Serial1.write((uint8_t)'A');
  delay(30);
  int r1 = Serial1.read();
  check("write(uint8_t) returns 1 and round-trips", n1 == 1 && r1 == 'A');

  size_t n2 = Serial1.write("Hello");
  delay(30);
  char buf[8] = {0};
  int i = 0;
  while (Serial1.available() && i < 7)
    buf[i++] = (char)Serial1.read();
  check("write(const char*) returns strlen and round-trips", n2 == 5 && strcmp(buf, "Hello") == 0);

  uint8_t raw[4] = {0x41, 0x00, 0x42, 0xFF};  // includes a null byte
  size_t n3 = Serial1.write(raw, sizeof(raw));
  delay(30);
  uint8_t rawback[4] = {0};
  int j = 0;
  while (Serial1.available() && j < 4)
    rawback[j++] = (uint8_t)Serial1.read();
  bool rawMatch = (j == 4) && rawback[0] == 0x41 && rawback[1] == 0x00 && rawback[2] == 0x42 && rawback[3] == 0xFF;
  check("write(buffer, size) handles null byte, count-based", n3 == 4 && rawMatch);
}

void loop() {
}
