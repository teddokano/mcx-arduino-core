/** AVR-era helpers that sketches and libraries use without a second thought:
 *  itoa()/utoa()/ltoa()/ultoa()/dtostrf(), word(h, l)/makeWord(), _BV(),
 *  analogReference()'s mode names, and the HardwareSerial type name.
 *
 *  No wiring. Read the final "ALL PASS"/"N FAIL" line.
 *
 *  Note that int is 32 bits here, not AVR's 16, so itoa(-1, s, 16) gives
 *  "ffffffff" rather than AVR's "ffff".
 */

#include <Arduino.h>
#include <cstring>

int fails = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    fails++;
}

// Compare a formatted string, printing what came out so a FAIL says why.
void check_str(const char *label, const char *got, const char *expect) {
  Serial.print(label);
  Serial.print(" -> \"");
  Serial.print(got);
  Serial.print("\": ");
  bool ok = strcmp(got, expect) == 0;
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    fails++;
}

// A library-style function taking the type other cores call Serial by.
size_t greet(HardwareSerial &port) {
  return port.println("hello through a HardwareSerial&");
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("AVR-compatible helpers test");
  char s[40];

  Serial.println("--- itoa / utoa / ltoa / ultoa ---");
  check_str("itoa(-123, 10)", itoa(-123, s, 10), "-123");
  check_str("itoa(255, 16)", itoa(255, s, 16), "ff");
  check_str("itoa(-1, 16)", itoa(-1, s, 16), "ffffffff");
  check_str("utoa(255, 2)", utoa(255, s, 2), "11111111");
  check_str("ltoa(-2147483648, 10)", ltoa(-2147483647L - 1, s, 10), "-2147483648");
  check_str("ltoa(35, 36)", ltoa(35L, s, 36), "z");
  check_str("ultoa(4294967295, 10)", ultoa(4294967295UL, s, 10), "4294967295");
  check_str("ultoa(4294967295, 16)", ultoa(4294967295UL, s, 16), "ffffffff");
  check("returns its buffer", ltoa(7L, s, 10) == s);

  Serial.println("--- dtostrf ---");
  check_str("dtostrf(3.14159, 7, 2)", dtostrf(3.14159, 7, 2, s), "   3.14");
  check_str("dtostrf(1.999, 1, 2) rounds", dtostrf(1.999, 1, 2, s), "2.00");
  check_str("dtostrf(-0.26, -7, 1) left-aligned", dtostrf(-0.26, -7, 1, s), "-0.3   ");
  check_str("dtostrf(1234.5678, 0, 3)", dtostrf(1234.5678, 0, 3, s), "1234.568");
  check_str("dtostrf(-3.0, 5, 0)", dtostrf(-3.0, 5, 0, s), "   -3");
  check_str("dtostrf(0.001, 0, 4)", dtostrf(0.001, 0, 4, s), "0.0010");
  check("returns its buffer", dtostrf(1.0, 0, 1, s) == s);

  Serial.println("--- word / makeWord / _BV ---");
  check("word(0x12, 0x34) == 0x1234", word(0x12, 0x34) == 0x1234);
  check("makeWord(0xAB, 0xCD) == 0xABCD", makeWord(0xAB, 0xCD) == 0xABCD);
  check("word(0x1234) == 0x1234", word(0x1234) == 0x1234);
  word w = 5000;  // the plain type name still works next to the macro
  check("word as a type", w == 5000 && sizeof(word) == 2);
  check("_BV(0) == 1", _BV(0) == 1);
  check("_BV(5) == 32", _BV(5) == 32);
  check("_BV(31) == 0x80000000", _BV(31) == 0x80000000UL);

  Serial.println("--- analogReference mode names ---");
  analogReference(DEFAULT);
  analogReference(INTERNAL);
  analogReference(EXTERNAL);
  analogReference(AR_DEFAULT);
  analogReference(AR_INTERNAL);
  analogReference(AR_EXTERNAL);
  analogReference(DEFAULT);
  int a = analogRead(A2);  // A2 is wired to the ADC on both boards
  check("analogRead(A2) still in range after analogReference()", a >= 0 && a <= 1023);

  Serial.println("--- HardwareSerial ---");
  size_t n = greet(Serial);
  check("HardwareSerial& is Serial, print() counts bytes", n == strlen("hello through a HardwareSerial&") + 2);

  Serial.println();
  if (fails) {
    Serial.print(fails);
    Serial.println(" FAIL");
  } else {
    Serial.println("ALL PASS");
  }
}

void loop() {
}
