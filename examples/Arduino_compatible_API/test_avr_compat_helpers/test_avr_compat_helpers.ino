/** AVR-era helpers that sketches and libraries use without a second thought:
 *  itoa()/utoa()/ltoa()/ultoa()/dtostrf(), word(h, l)/makeWord(), _BV(),
 *  analogReference()'s mode names, the HardwareSerial type name, avr-libc's
 *  BSD/POSIX string functions (strlcpy() and the rest) and M_PI and the
 *  other M_ constants, and BitOrder as a real enum.
 *
 *  No wiring. Read the final "ALL PASS"/"N FAIL" line.
 *
 *  Note that int is 32 bits here, not AVR's 16, so itoa(-1, s, 16) gives
 *  "ffffffff" rather than AVR's "ffff".
 */

#include <Arduino.h>
#include <cstring>

// Names a sketch may use for its own globals, as on AVR. Building at all is
// the check: exposing the BSD/XSI parts of newlib's headers wholesale
// (-std=gnu++20) would clash them with index() and y0()/y1().
int index = 0;
int x0 = 1, y0 = 2, x1 = 3, y1 = 4;

// Overloaded on BitOrder, as Adafruit BusIO's Adafruit_SPIDevice
// constructors are. With BitOrder a plain integer, a call passing MSBFIRST
// matched both equally well and failed to build.
int order_kind(BitOrder) { return 1; }
int order_kind(int8_t) { return 2; }

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

  Serial.println("--- avr-libc string functions ---");
  char t[8];
  check("strlcpy truncates and returns strlen(src)", strlcpy(t, "abcdefghij", sizeof t) == 10 && strcmp(t, "abcdefg") == 0);
  strlcpy(t, "ab", sizeof t);
  check("strlcat truncates and returns the length it tried", strlcat(t, "cdefghij", sizeof t) == 10 && strcmp(t, "abcdefg") == 0);
  char *d = strdup("dup");
  check("strdup", d != nullptr && strcmp(d, "dup") == 0);
  free(d);
  d = strndup("dupdup", 3);
  check("strndup", d != nullptr && strcmp(d, "dup") == 0);
  free(d);
  strlcpy(s, "a,b,,c", sizeof s);
  char *save;
  char *t1 = strtok_r(s, ",", &save);
  char *t2 = strtok_r(nullptr, ",", &save);
  char *t3 = strtok_r(nullptr, ",", &save);
  check("strtok_r skips the empty field", strcmp(t1, "a") == 0 && strcmp(t2, "b") == 0 && strcmp(t3, "c") == 0 && strtok_r(nullptr, ",", &save) == nullptr);
  strlcpy(s, "a,b,,c", sizeof s);
  char *p = s;
  char *f1 = strsep(&p, ",");
  char *f2 = strsep(&p, ",");
  char *f3 = strsep(&p, ",");
  check("strsep keeps the empty field", strcmp(f1, "a") == 0 && strcmp(f2, "b") == 0 && strcmp(f3, "") == 0 && strcmp(p, "c") == 0);
  check("strnlen", strnlen("abcdef", 4) == 4 && strnlen("ab", 4) == 2);
  char *after = (char *)memccpy(t, "xy:z", ':', sizeof t);
  check("memccpy stops after the ':'", after == t + 3 && memcmp(t, "xy:", 3) == 0);
  const char hay[] = "haystack";
  check("memmem", memmem(hay, 8, "st", 2) == hay + 3);
  check("strcasestr", strcasestr("Hello World", "WORLD") == strstr("Hello World", "World"));

  Serial.println("--- M_ constants ---");
  check("M_PI == PI", M_PI == PI);
  check("M_PI_2, M_2_PI, M_1_PI", fabs(M_PI_2 * 2 - M_PI) < 1e-15 && fabs(M_2_PI * M_PI - 2) < 1e-15 && fabs(M_1_PI * M_PI - 1) < 1e-15);
  check("M_E, M_LN2, M_LN10", fabs(log(M_E) - 1) < 1e-15 && fabs(exp(M_LN2) - 2) < 1e-14 && fabs(exp(M_LN10) - 10) < 1e-13);
  check("M_SQRT2, M_SQRT1_2", fabs(M_SQRT2 * M_SQRT2 - 2) < 1e-15 && fabs(M_SQRT1_2 * M_SQRT2 - 1) < 1e-15);

  Serial.println("--- BitOrder, and names left free ---");
  check("MSBFIRST picks the BitOrder overload", order_kind(MSBFIRST) == 1 && order_kind(LSBFIRST) == 1);
  check("LSBFIRST == 0, MSBFIRST == 1", LSBFIRST == 0 && MSBFIRST == 1);
  index++;
  check("globals index, x0, y0, x1, y1", index == 1 && x0 + y0 + x1 + y1 == 10);

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
