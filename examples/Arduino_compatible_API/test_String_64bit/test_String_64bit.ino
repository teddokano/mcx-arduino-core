/** String 64bit (long long / unsigned long long) constructor + concat test
 *
 *  Pure logic, no hardware needed. Uses values that overflow a 32bit long
 *  to confirm the 64bit path is actually being used, not silently
 *  narrowed through the 32bit long overload.
 */

#include <Arduino.h>

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("String 64bit test");

  long long bigNeg = -5000000000LL;         // overflows 32bit long
  unsigned long long bigPos = 10000000000ULL;  // overflows 32bit unsigned long

  String s1(bigNeg);
  Serial.println(s1);
  check("String(long long)", s1 == "-5000000000");

  String s2(bigPos);
  Serial.println(s2);
  check("String(unsigned long long)", s2 == "10000000000");

  String s3 = "value=";
  s3 += bigPos;
  Serial.println(s3);
  check("operator+=(unsigned long long)", s3 == "value=10000000000");

  String s4;
  s4.concat(bigNeg);
  check("concat(long long)", s4 == "-5000000000");

  // hex base
  String s5(255LL, HEX);
  check("String(long long, HEX)", s5 == "ff");
}

void loop() {
}
