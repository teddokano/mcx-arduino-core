/** Serial.print(double) / String(double): rounding and the edge cases.
 *
 *  Before v0.7.0 both cut digits off instead of rounding, so
 *  Serial.print(1.999) printed "1.99" where every other Arduino core prints
 *  "2.00", and print(x, 0) left a trailing "." behind.
 *
 *  Expected output follows ArduinoCore-avr/ArduinoCore-API: print() rounds
 *  by adding half of the last digit, prints "nan"/"inf", and prints "ovf"
 *  once the integer part won't fit 32 bits. String(double) is built on
 *  %f, as theirs is on dtostrf(), so it prints large values in full.
 *
 *  No wiring. Read the final "ALL PASS"/"N FAIL" line.
 */

#include <Arduino.h>
#include <cstring>

// Collects whatever is print()ed into it, so the exact text can be compared.
class Capture : public Print {
public:
  size_t write(uint8_t c) override {
    if (len < sizeof(buf) - 1) {
      buf[len++] = (char)c;
      buf[len] = '\0';
    }
    return 1;
  }
  const char *take() {
    len = 0;
    return buf;
  }
  char buf[64] = "";
  size_t len = 0;
};

Capture cap;
int fails = 0;

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

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    fails++;
}

void print_case(const char *label, double v, int digits, const char *expect) {
  size_t n = cap.print(v, digits);
  const char *got = cap.take();
  check_str(label, got, expect);
  check("  return value is the length", n == strlen(expect));
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("print(double) / String(double) rounding test");

  Serial.println("--- print(double) ---");
  cap.print(1.999);
  check_str("print(1.999)", cap.take(), "2.00");
  print_case("print(3.14159, 2)", 3.14159, 2, "3.14");
  print_case("print(3.14159, 4)", 3.14159, 4, "3.1416");
  print_case("print(-3.14159, 3)", -3.14159, 3, "-3.142");
  print_case("print(0.125, 2)", 0.125, 2, "0.13");
  print_case("print(9.9999, 2) carries into the integer", 9.9999, 2, "10.00");
  print_case("print(1.999, 0) has no decimal point", 1.999, 0, "2");
  print_case("print(2.5, 0)", 2.5, 0, "3");
  print_case("print(0.0, 2)", 0.0, 2, "0.00");
  print_case("print(-0.001, 2)", -0.001, 2, "-0.00");
  print_case("print(123456.789, 1)", 123456.789, 1, "123456.8");
  print_case("print(4000000000.0, 0) (beyond long)", 4000000000.0, 0, "4000000000");
  print_case("print(5e9) is ovf", 5e9, 2, "ovf");
  print_case("print(-5e9) is ovf", -5e9, 2, "ovf");
  print_case("print(NAN)", NAN, 2, "nan");
  print_case("print(INFINITY)", INFINITY, 2, "inf");
  print_case("print(float 0.1f, 3)", 0.1f, 3, "0.100");

  size_t n = cap.println(1.5, 0);
  check_str("println(1.5, 0)", cap.take(), "2\r\n");
  check("  return value counts the line ending", n == 3);

  Serial.println("--- String(double) ---");
  check_str("String(1.999)", String(1.999).c_str(), "2.00");
  check_str("String(1.999f, 1)", String(1.999f, 1).c_str(), "2.0");
  check_str("String(3.14159, 4)", String(3.14159, 4).c_str(), "3.1416");
  check_str("String(-2.345678, 3)", String(-2.345678, 3).c_str(), "-2.346");
  check_str("String(5.0, 0)", String(5.0, 0).c_str(), "5");
  check_str("String(1e10, 1) in full", String(1e10, 1).c_str(), "10000000000.0");
  check_str("String(1e30, 0) in full", String(1e30, 0).c_str(), "1000000000000000019884624838656");
  check_str("\"v=\" + 1.999", (String("v=") + 1.999).c_str(), "v=2.00");
  String s = "t=";
  s += 21.996f;
  check_str("+= 21.996f", s.c_str(), "t=22.00");

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
