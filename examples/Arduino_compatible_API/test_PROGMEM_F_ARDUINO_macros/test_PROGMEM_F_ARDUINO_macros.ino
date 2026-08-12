/** PROGMEM/pgm_read_*, F()/__FlashStringHelper, ARDUINO/ARDUINO_ARCH_*
 *  compatibility macros test. Pure logic + compile-time checks, no
 *  hardware needed.
 */

#include <Arduino.h>
#include <cstring>

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
}

const uint8_t testData[] PROGMEM = { 0x11, 0x22, 0x33, 0x44 };

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("PROGMEM / F() / ARDUINO macros test");

  // ---- PROGMEM / pgm_read_byte ----
  uint8_t b0 = pgm_read_byte(&testData[0]);
  uint8_t b2 = pgm_read_byte(&testData[2]);
  check("pgm_read_byte", b0 == 0x11 && b2 == 0x33);

  // ---- F() / __FlashStringHelper ----
  Serial.println(F("hello from F()"));
  Serial.print(F("F() with print: "));
  Serial.println(42);

  String s1 = String(F("flash-string"));
  check("String(F(...))", s1 == "flash-string");

  String s2 = "prefix-";
  s2 += F("suffix");
  check("String += F(...)", s2 == "prefix-suffix");

  // ---- ARDUINO version macro ----
#if defined(ARDUINO) && ARDUINO >= 100
  check("ARDUINO defined and >= 100", true);
#else
  check("ARDUINO defined and >= 100", false);
#endif

  // ---- ARDUINO_ARCH_* / ARDUINO_<BOARD> ----
#if defined(ARDUINO_ARCH_MCX)
  check("ARDUINO_ARCH_MCX defined", true);
#else
  check("ARDUINO_ARCH_MCX defined", false);
#endif

#if defined(ARDUINO_FRDM_MCXA153)
  check("ARDUINO_FRDM_MCXA153 defined", true);
#else
  check("ARDUINO_FRDM_MCXA153 defined", false);
#endif
}

void loop() {
}
