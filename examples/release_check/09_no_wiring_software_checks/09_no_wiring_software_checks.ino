/** Release check 09: automatic OK/FAIL checks of the core's software API,
 *  no physical wiring needed.
 *
 *  Split out of release_check/01 in 0.9.0, which keeps the checks that use
 *  a peripheral: 01 had grown to fill all but 40 bytes of FRDM-MCXA153's
 *  flash. Nothing here touches a peripheral beyond Serial for the report,
 *  so it runs the same on every board.
 *
 *  Consolidates (from examples/Arduino_compatible_API/): test_math_constants,
 *  test_arduino_compat_macros, test_avr_compat_helpers,
 *  test_MOSI_MISO_SCK_macros, test_Print_writeError, test_String,
 *  test_String_64bit, test_Serial_print_time_t, test_function_local_static
 *  (since 0.9.1), and the representative checks of test_print_float_rounding.
 *
 *  Read the final "ALL OK"/"N FAILED" line.
 */

#include <Arduino.h>
#include <cstring>

// A minimal Print-derived class that can simulate a failing write(), used
// by the Print::*WriteError() section below. File-scope, not declared
// inside setup() -- a local class with a virtual override here tripped a
// placement-new/char_traits<char32_t> compile error in this toolchain
// that the identical class at file scope doesn't.
class FlakyPrint : public Print {
public:
  size_t write(uint8_t c) override {
    if (fail) {
      setWriteError();
      return 0;
    }
    return 1;
  }
  bool fail = false;
};

// For the AVR-era helpers section: globals a sketch may name as on AVR
// (building at all is the check), and an overload on BitOrder, which
// Adafruit BusIO has and which a plain-integer BitOrder made ambiguous.
int index = 0;
int x0 = 1, y0 = 2, x1 = 3, y1 = 4;
int order_kind(BitOrder) { return 1; }
int order_kind(int8_t) { return 2; }

// For the function-local statics section: statics made the first time
// their function runs. Up to 0.9.0 these failed to link (__cxa_guard_*).
int lsConstructed = 0;
struct LocalStaticCounter {
  int n = 0;
  LocalStaticCounter() { lsConstructed++; }
  ~LocalStaticCounter() {}
  int next() { return ++n; }
};
int lsNextCount() {
  static LocalStaticCounter c;
  return c.next();
}
int lsInitialCalls = 0;
int lsInitial() {
  lsInitialCalls++;
  return 100;
}
int lsNextFromInitial() {
  static int v = lsInitial();
  return ++v;
}

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("=== Release check 09: no-wiring software checks ===");

  // ---- math constants / trig (was test_math_constants) ----
  Serial.println("--- math constants ---");
  check("PI", fabs(PI - 3.14159265) < 0.00001);
  check("HALF_PI", fabs(HALF_PI - 1.57079633) < 0.00001);
  check("TWO_PI", fabs(TWO_PI - 6.28318531) < 0.00001);
  check("radians(180) == PI", fabs(radians(180.0) - PI) < 0.00001);
  check("degrees(PI) == 180", fabs(degrees(PI) - 180.0) < 0.00001);
  check("sin(HALF_PI) == 1", fabs(sin(HALF_PI) - 1.0) < 0.00001);
  check("sqrt(2.0)", fabs(sqrt(2.0) - 1.41421356) < 0.00001);

  // ---- UNO R3/R4 compat macros (was test_arduino_compat_macros) ----
  Serial.println("--- compat macros ---");
  check("min(3,7)", min(3, 7) == 3);
  check("max(3,7)", max(3, 7) == 7);
  check("abs(-5)", abs(-5) == 5);
  check("constrain(15,0,10)", constrain(15, 0, 10) == 10);
  check("sq(4)", sq(4) == 16);
  check("map(512,0,1023,0,255)", map(512, 0, 1023, 0, 255) == 127);

  uint8_t v = 0;
  bitSet(v, 3);
  check("bitSet/bitRead", bitRead(v, 3) == 1);
  bitClear(v, 3);
  check("bitClear", bitRead(v, 3) == 0);

  check("lowByte(0x1234)", lowByte(0x1234) == 0x34);
  check("highByte(0x1234)", highByte(0x1234) == 0x12);
  check("bit(3)", bit(3) == 8);

  noInterrupts();
  interrupts();
  check("interrupts()/noInterrupts() (reached here without hanging)", true);

  byte b = 200;
  word w = 5000;
  boolean flag = true;
  check("byte/word/boolean types", b == 200 && w == 5000 && flag == true);

  // ---- AVR-era helpers (was test_avr_compat_helpers) ----
  Serial.println("--- AVR-era helpers ---");
  {
    char s[40];
    check("itoa(-123, 10)", strcmp(itoa(-123, s, 10), "-123") == 0);
    check("itoa(-1, 16) (32-bit int)", strcmp(itoa(-1, s, 16), "ffffffff") == 0);
    check("utoa(255, 2)", strcmp(utoa(255, s, 2), "11111111") == 0);
    check("ltoa(-2147483648, 10)", strcmp(ltoa(-2147483647L - 1, s, 10), "-2147483648") == 0);
    check("ultoa(4294967295, 16)", strcmp(ultoa(4294967295UL, s, 16), "ffffffff") == 0);
    check("dtostrf(3.14159, 7, 2)", strcmp(dtostrf(3.14159, 7, 2, s), "   3.14") == 0);
    check("dtostrf(1.999, 1, 2) rounds", strcmp(dtostrf(1.999, 1, 2, s), "2.00") == 0);
    check("dtostrf(-0.26, -7, 1) left-aligned", strcmp(dtostrf(-0.26, -7, 1, s), "-0.3   ") == 0);
    check("word(0x12, 0x34)", word(0x12, 0x34) == 0x1234);
    check("_BV(5)", _BV(5) == 32);
    analogReference(DEFAULT);
    analogReference(AR_DEFAULT);
    check("analogReference(DEFAULT/AR_DEFAULT) accepted", true);
    HardwareSerial &port = Serial;
    check("HardwareSerial& refers to Serial", &port == &Serial);
    char t[8];
    check("strlcpy truncates", strlcpy(t, "abcdefghij", sizeof t) == 10 && strcmp(t, "abcdefg") == 0);
    strlcpy(s, "a,b,,c", sizeof s);
    char *p = s;
    strsep(&p, ",");
    strsep(&p, ",");
    check("strsep keeps the empty field", strcmp(strsep(&p, ","), "") == 0 && strcmp(p, "c") == 0);
    char *save;
    check("strtok_r", strcmp(strtok_r(s, ",", &save), "a") == 0);
    check("strnlen", strnlen("abcdef", 4) == 4);
    check("M_PI == PI", M_PI == PI);
    check("MSBFIRST picks the BitOrder overload", order_kind(MSBFIRST) == 1);
    index++;
    check("globals index, x0, y0, x1, y1", index == 1 && x0 + y0 + x1 + y1 == 10);
  }

  // ---- MOSI/MISO/SCK bare macros (was test_MOSI_MISO_SCK_macros) ----
  Serial.println("--- MOSI/MISO/SCK macros ---");
  check("MOSI == ARD_MOSI", MOSI == ARD_MOSI);
  check("MISO == ARD_MISO", MISO == ARD_MISO);
  check("SCK == ARD_SCK", SCK == ARD_SCK);

  // ---- Print::set/get/clearWriteError() (was test_Print_writeError) ----
  Serial.println("--- Print::*WriteError() ---");
  {
    FlakyPrint flaky;
    check("initial getWriteError() == 0", flaky.getWriteError() == 0);
    flaky.fail = true;
    flaky.print("x");
    check("getWriteError() != 0 after failed write", flaky.getWriteError() != 0);
    flaky.clearWriteError();
    check("getWriteError() == 0 after clearWriteError()", flaky.getWriteError() == 0);
    flaky.fail = false;
    flaky.print("x");
    check("getWriteError() == 0 after successful write", flaky.getWriteError() == 0);
    // print(double) rounds (was test_print_float_rounding): "10.00" is 5
    // bytes where truncating gave "9.99", and "2" is 1 where it gave "1."
    check("print(9.9999) rounds up to \"10.00\"", flaky.print(9.9999) == 5);
    check("print(1.999, 0) is \"2\", no decimal point", flaky.print(1.999, 0) == 1);
  }

  // ---- String (was test_String) ----
  Serial.println("--- String ---");
  {
    String a = "Hello";
    String b = String(", ") + "world" + String('!');
    String c = a + b;
    check("concat", c == "Hello, world!");

    String num = String(42) + " / " + String(3.14, 2);
    check("numeric concat", num == "42 / 3.14");
    check("String(1.999) rounds to \"2.00\" (was test_print_float_rounding)", String(1.999) == "2.00");

    check("length", c.length() == 13);
    check("charAt", c.charAt(0) == 'H');
    check("indexOf", c.indexOf("world") == 7);
    check("substring", c.substring(7, 12) == "world");
    check("startsWith", c.startsWith("Hello"));
    check("endsWith", c.endsWith("!"));

    String upper = c;
    upper.toUpperCase();
    check("toUpperCase", upper == "HELLO, WORLD!");

    String spaced = "   trim me   ";
    spaced.trim();
    check("trim", spaced == "trim me");

    String replaced = c;
    replaced.replace("world", "there");
    check("replace", replaced == "Hello, there!");

    check("toInt", String("12345").toInt() == 12345);

    float ft = String("3.5").toFloat();
    check("toFloat", ft > 3.49f && ft < 3.51f);
  }

  // ---- String 64bit (was test_String_64bit) ----
  Serial.println("--- String 64bit ---");
  {
    long long bigNeg = -5000000000LL;
    unsigned long long bigPos = 10000000000ULL;

    check("String(long long)", String(bigNeg) == "-5000000000");
    check("String(unsigned long long)", String(bigPos) == "10000000000");

    String s3 = "value=";
    s3 += bigPos;
    check("operator+=(unsigned long long)", s3 == "value=10000000000");

    String s4;
    s4.concat(bigNeg);
    check("concat(long long)", s4 == "-5000000000");

    check("String(long long, HEX)", String(255LL, HEX) == "ff");
  }

  // ---- function-local statics made at run time (was
  //      test_function_local_static) -- building at all is the first check ----
  Serial.println("--- function-local statics ---");
  {
    int a = lsNextCount();
    int b = lsNextCount();
    int c = lsNextCount();
    check("object with a constructor keeps its state, constructed once",
          a == 1 && b == 2 && c == 3 && lsConstructed == 1);
    int d = lsNextFromInitial();
    int e = lsNextFromInitial();
    check("value set from a function call keeps its state, set once",
          d == 101 && e == 102 && lsInitialCalls == 1);
  }

  // ---- Serial.print(time_t)/(long long)/(unsigned long long) overload
  //      resolution (was test_Serial_print_time_t) -- the original point
  //      of this test is that these lines *compile* at all (time_t is
  //      ambiguous between long/long long on this newlib), so there's no
  //      further runtime check beyond that; printed for a human to glance
  //      at if something looks wrong.
  Serial.println("--- Serial.print(time_t / long long / unsigned long long) ---");
  time_t current_time = 1734567890;
  Serial.print("time_t: ");
  Serial.println(current_time);
  Serial.print("long long: ");
  Serial.println(123456789012345LL);
  Serial.print("unsigned long long: ");
  Serial.println(18446744073709551615ULL);

  Serial.println();
  if (failCount == 0)
    Serial.println("ALL OK");
  else {
    Serial.print(failCount);
    Serial.println(" FAILED");
  }
}

void loop() {
}
