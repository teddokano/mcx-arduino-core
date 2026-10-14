/** avr-libc's <avr/pgmspace.h>, as AVR-era sketches and libraries use it:
 *  PROGMEM data read back with pgm_read_*() and their _near/_far forms,
 *  the _P string and memory functions with PSTR() strings, the _PF forms
 *  with pgm_get_far_address(), and the _P forms of sprintf/sscanf.
 *
 *  Flash is ordinary memory on this chip, so all of these are plain reads
 *  and the ordinary string functions; the sketch checks that each one is
 *  there and gives the AVR result.
 *
 *  No wiring. Read the final "ALL PASS"/"N FAIL" line.
 */

#include <Arduino.h>
#include <avr/pgmspace.h>  // Arduino.h already includes it, as on AVR; this must still work
#include <stdio.h>         // for sprintf_P()/sscanf_P(), as on AVR

// A library's usual guard: only define it if the core doesn't
#ifndef pgm_read_byte_near
#error "pgm_read_byte_near is not a macro"
#endif
#ifndef memcpy_P
#error "memcpy_P is not a macro"
#endif

const uint8_t bytes[] PROGMEM = { 0x11, 0x22, 0x33, 0x44 };
const uint16_t words[] PROGMEM = { 0x1234, 0xBEEF };
const uint32_t dwords[] PROGMEM = { 0xDEADBEEF };
const uint64_t qwords[] PROGMEM = { 0x0123456789ABCDEFULL };
const float floats[] PROGMEM = { 2.5f };
const char greeting[] PROGMEM = "Hello, PROGMEM";
const char item0[] PROGMEM = "zero";
const char item1[] PROGMEM = "one";
const char *const items[] PROGMEM = { item0, item1 };
const prog_uchar old_style[] PROGMEM = { 7, 8, 9 };

int fails = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    fails++;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("avr/pgmspace.h test");

  Serial.println("--- pgm_read_* ---");
  check("pgm_read_byte", pgm_read_byte(&bytes[2]) == 0x33);
  check("pgm_read_byte_near", pgm_read_byte_near(bytes + 3) == 0x44);
  check("pgm_read_byte_far", pgm_read_byte_far(pgm_get_far_address(bytes[1])) == 0x22);
  check("pgm_read_word / _near / _far", pgm_read_word(&words[1]) == 0xBEEF && pgm_read_word_near(&words[0]) == 0x1234 && pgm_read_word_far(pgm_get_far_address(words[1])) == 0xBEEF);
  check("pgm_read_dword / _near / _far", pgm_read_dword(&dwords[0]) == 0xDEADBEEF && pgm_read_dword_near(dwords) == 0xDEADBEEF && pgm_read_dword_far(pgm_get_far_address(dwords[0])) == 0xDEADBEEF);
  check("pgm_read_qword", pgm_read_qword(&qwords[0]) == 0x0123456789ABCDEFULL && pgm_read_qword_near(qwords) == 0x0123456789ABCDEFULL);
  check("pgm_read_float / _near / _far", pgm_read_float(&floats[0]) == 2.5f && pgm_read_float_near(floats) == 2.5f && pgm_read_float_far(pgm_get_far_address(floats[0])) == 2.5f);
  check("pgm_read_ptr: table of strings", strcmp((const char *)pgm_read_ptr(&items[1]), "one") == 0);
  check("pgm_read_ptr_near / _far", pgm_read_ptr_near(&items[0]) == item0 && pgm_read_ptr_far(pgm_get_far_address(items[1])) == item1);
  void *as_void = pgm_read_ptr(&items[0]);  // void *, as in avr-libc
  check("pgm_read_ptr gives void *", as_void == item0);
  check("prog_uchar", pgm_read_byte(&old_style[2]) == 9);
  PGM_P p = greeting;
  PGM_VOID_P vp = bytes;
  check("PGM_P / PGM_VOID_P", pgm_read_byte(p) == 'H' && pgm_read_byte(vp) == 0x11);

  Serial.println("--- _P string and memory functions ---");
  char buf[40];
  check("strlen_P", strlen_P(greeting) == 14);
  check("strnlen_P", strnlen_P(greeting, 5) == 5);
  check("strcpy_P", strcmp(strcpy_P(buf, greeting), "Hello, PROGMEM") == 0);
  check("strncpy_P", (strncpy_P(buf, PSTR("abcdef"), 3), buf[3] = '\0', strcmp(buf, "abc") == 0));
  strcpy_P(buf, PSTR("ab"));
  check("strcat_P", strcmp(strcat_P(buf, PSTR("cd")), "abcd") == 0);
  check("strncat_P", strcmp(strncat_P(buf, PSTR("efgh"), 2), "abcdef") == 0);
  check("strlcpy_P", strlcpy_P(buf, greeting, 6) == 14 && strcmp(buf, "Hello") == 0);
  check("strlcat_P", strlcat_P(buf, PSTR(", world"), sizeof(buf)) == 12 && strcmp(buf, "Hello, world") == 0);
  check("strcmp_P", strcmp_P("Hello, PROGMEM", greeting) == 0 && strcmp_P("A", PSTR("B")) < 0);
  check("strncmp_P", strncmp_P("Hello!", greeting, 5) == 0);
  check("strcasecmp_P", strcasecmp_P("hello, progmem", greeting) == 0);
  check("strncasecmp_P", strncasecmp_P("HELLO", greeting, 5) == 0);
  check("strstr_P", strstr_P("xxPROGMEMyy", PSTR("PROG")) != nullptr);
  check("strcasestr_P", strcasestr_P("xxprogmemyy", PSTR("PROG")) != nullptr);
  check("strchr_P / strrchr_P", strchr_P(greeting, 'l') == greeting + 2 && strrchr_P(greeting, 'l') == greeting + 3);
  check("strchrnul_P", *strchrnul_P(greeting, '#') == '\0');
  check("strspn_P / strcspn_P", strspn_P("aab", PSTR("a")) == 2 && strcspn_P("abc", PSTR("c")) == 2);
  check("strpbrk_P", strpbrk_P("hello", PSTR("lo")) != nullptr);
  check("memcpy_P", (memcpy_P(buf, bytes, 4), memcmp(buf, "\x11\x22\x33\x44", 4) == 0));
  check("memcmp_P", memcmp_P("\x11\x22", bytes, 2) == 0);
  check("memchr_P / memrchr_P", memchr_P(bytes, 0x33, 4) == bytes + 2 && memrchr_P(bytes, 0x11, 4) == bytes);
  check("memmem_P", memmem_P(greeting, 14, "PROG", 4) == greeting + 7);
  check("memccpy_P", memccpy_P(buf, greeting, ',', 14) == buf + 6);
  strcpy(buf, "a,b,c");
  char *rest = buf;
  check("strsep_P", strcmp(strsep_P(&rest, PSTR(",")), "a") == 0);
  strcpy(buf, "x y");
  check("strtok_P", strcmp(strtok_P(buf, PSTR(" ")), "x") == 0);
  strcpy(buf, "u v");
  char *last;
  check("strtok_rP", strcmp(strtok_rP(buf, PSTR(" "), &last), "u") == 0);

  Serial.println("--- _PF forms ---");
  uint_farptr_t far = pgm_get_far_address(greeting);
  check("strlen_PF / strnlen_PF", strlen_PF(far) == 14 && strnlen_PF(far, 3) == 3);
  check("strcpy_PF / strcmp_PF", (strcpy_PF(buf, far), strcmp_PF(buf, far) == 0));
  check("memcpy_PF / memcmp_PF", (memcpy_PF(buf, pgm_get_far_address(bytes), 4), memcmp_PF(buf, pgm_get_far_address(bytes), 4) == 0));

  Serial.println("--- printf family ---");
  sprintf_P(buf, PSTR("%d-%s"), 42, "x");
  check("sprintf_P", strcmp(buf, "42-x") == 0);
  snprintf_P(buf, 4, PSTR("%s"), "abcdef");
  check("snprintf_P", strcmp(buf, "abc") == 0);
  int a = 0, b = 0;
  check("sscanf_P", sscanf_P("7 9", PSTR("%d %d"), &a, &b) == 2 && a == 7 && b == 9);

  Serial.println();
  if (fails == 0)
    Serial.println("ALL PASS");
  else {
    Serial.print(fails);
    Serial.println(" FAIL");
  }
}

void loop() {
}
